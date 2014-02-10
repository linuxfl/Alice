/**
 * @file entity.cpp
 * @author Robin Dietrich <me (at) invokr (dot) org>
 * @version 1.0
 *
 * @par License
 *    Alice Replay Parser
 *    Copyright 2014 Robin Dietrich
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 */

#include <alice/entity.hpp>

namespace dota {
    void entity::updateFromBitstream(bitstream& bstream) {
        // use this static vector so we don't realocate memory all the time
        static std::vector<uint32_t> fields(1000, 0);
        fields.clear();

        uint32_t fieldId = -1;

        /** small lambda reading the field id from the bitstream */
        auto readFieldId = [&]() {
            const bool incremental = bstream.read(1);
            if (incremental) {
                ++fieldId;
            } else {
                const uint32_t value = bstream.readVarUInt();

                if (value == 0x3FFF)
                    fieldId = 0xFFFFFFFF;
                else
                    fieldId += value + 1;
            }
        };

        readFieldId();
        while (fieldId != 0xFFFFFFFF) {
            fields.push_back(fieldId);
            readFieldId();
        }

        for (auto &it : fields) {
            //const std::string &name = stringMap[it];
            auto i = properties.find(it);
            if (i == properties.end())
                properties[it] = property::create(bstream, flat->properties[it]);
            else
                i->second.update(bstream);
        }
    }

    std::string entity::DebugString() {
        std::stringstream str("");

        str << "Name: " << cls->networkName << " / Id: " << id << " / State: " << currentState << std::endl;

        for (auto &p : properties) {
            str << "Property: " << p.first << " / Type: ";

            switch (p.second.getType()) {
                case sendprop::T_Int:
                    str << " Int / Value: ";
                    break;
                case sendprop::T_Float:
                    str << " Float / Value: ";
                    break;
                case sendprop::T_Vector: {
                    str << " Vector / Value: ";
                } break;
                case sendprop::T_VectorXY: {
                    str << " VectorXY / Value: ";
                } break;
                case sendprop::T_String:
                    str << " String / Value: ";
                    break;
                case sendprop::T_Array:
                    str << " Array / Value: ";
                    break;
                case sendprop::T_DataTable:
                    str << " DataTable / Value: ";
                    break;
                default:
                    str << " Unkown" << std::endl;
            }

            str << p.second.asString() << std::endl;
        }

        return str.str();
    }

    void entity::readHeader(uint32_t &id, bitstream &bstream, state_type &type) {
        uint32_t tmp = bstream.read(6);
        if (tmp & 0x30) {
            const uint32_t x1 = (tmp >> 4) & 3;
            const uint32_t x2 = (x1 == 3) ? 16 : 0;

            tmp = bstream.read(4 * x1 + x2) << 4 | (tmp & 0xF);
        }

        id += tmp + 1;

        // In Source the entity state is referred to as PVS (Potential Visible State) which
        // can have a number of different configurations.
        //
        // We only care about what we should do with the entity so we only represent those three
        // states: Create, Update and Delete.
        //
        // Cases in which entities are marked to leave the PVS but are not explicitly deleted are not
        // handled as they only represent a state change which we do not need to take into account being
        // not a client. In Source this might trigger behavior like dropping the entity after a certain
        // time which is something we don't need to take care of.
        //
        // If you want to get a more accurate representation of these states you should have a look at
        // edith or skadi.

        if (!bstream.read(1)) {
            if (bstream.read(1)) {
                type = state_created;
            } else {
                type = state_updated;
            }
        } else if (bstream.read(1)) {
            type = state_deleted;
        } else {
            type = state_default;
        }
    }
}