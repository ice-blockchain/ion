/*
    This file is part of ION Blockchain Library.

    ION Blockchain Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    ION Blockchain Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with ION Blockchain Library.  If not, see <http://www.gnu.org/licenses/>.

    Copyright 2017-2020 Telegram Systems LLP
*/
#pragma once

#include "ion/ion-types.h"
#include "auto/tl/ion_api.h"
#include "keys/keys.hpp"

namespace ion {

namespace validator {

class ValidatorFullId : public PublicKey {
 public:
  NodeIdShort short_id() const;
  operator Ed25519_PublicKey() const;

  ValidatorFullId(PublicKey id) : PublicKey{std::move(id)} {
  }
  ValidatorFullId(const Ed25519_PublicKey& key) : PublicKey{pubkeys::Ed25519{key.as_bits256()}} {
  }
};

}  // namespace validator

}  // namespace ion
