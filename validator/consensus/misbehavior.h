/*
 * Copyright (c) 2025-2026, ION CORE TECHNOLOGIES CO. L.L.C
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "crypto/common/refcnt.hpp"

namespace ion::validator::consensus {

class Misbehavior : public td::CntObject {
 public:
  virtual ~Misbehavior() = default;
};

using MisbehaviorRef = td::Ref<Misbehavior>;

}  // namespace ion::validator::consensus
