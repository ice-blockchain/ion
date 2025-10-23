/*
    This file is part of TON Blockchain Library.

    TON Blockchain Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    TON Blockchain Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with TON Blockchain Library.  If not, see <http://www.gnu.org/licenses/>.

    Copyright 2017-2020 Telegram Systems LLP
*/
#include "td/utils/base64.h"
#include "td/utils/utf8.h"
#include "vm/cells/Cell.h"
#include "vm/cells/CellBuilder.h"
#include "tl/tlblib.hpp"
#include <iomanip>
#include <vector>


namespace tlb {

const False t_False;
const True t_True;
const Unit t_Unit;

const Bool t_Bool;

const Int t_int8{8}, t_int16{16}, t_int24{24}, t_int32{32}, t_int64{64}, t_int128{128}, t_int256{256}, t_int257{257};
const UInt t_uint8{8}, t_uint16{16}, t_uint24{24}, t_uint32{32}, t_uint64{64}, t_uint128{128}, t_uint256{256};
const NatWidth t_Nat{32};

const Anything t_Anything;
const RefAnything t_RefCell;

const SnakeString t_SnakeString;

std::string TLB::get_type_name() const {
  std::ostringstream os;
  print_type(os);
  return os.str();
}

bool Bool::print_skip(PrettyPrinter& pp, vm::CellSlice& cs) const {
  int t = get_tag(cs);
  return cs.advance(1) && pp.out(t ? "bool_true" : "bool_false");
}

bool Bool::print_skip(JsonPrinter& pp, vm::CellSlice& cs) const {
  int t = get_tag(cs);
  return cs.advance(1) && pp.out(t ? "bool_true" : "bool_false");
}

bool NatWidth::print_skip(PrettyPrinter& pp, vm::CellSlice& cs) const {
  long long value = (long long)cs.fetch_ulong(n);
  return value >= 0 && pp.out_int(value);
}

bool NatLeq::print_skip(PrettyPrinter& pp, vm::CellSlice& cs) const {
  long long value = (long long)as_uint(cs);
  return value >= 0 && skip(cs) && pp.out_int(value);
}

bool NatLess::print_skip(PrettyPrinter& pp, vm::CellSlice& cs) const {
  long long value = (long long)as_uint(cs);
  return value >= 0 && skip(cs) && pp.out_int(value);
}

bool TupleT::print_skip(PrettyPrinter& pp, vm::CellSlice& cs) const {
  pp.open("tuple ");
  pp.os << n << " [";
  pp.mode_nl();
  int i = n;
  for (; i > 0; --i) {
    if (!X.print_skip(pp, cs)) {
      return false;
    }
    pp.mode_nl();
  }
  return pp.close("]");
}

bool TupleT::print_skip(JsonPrinter& pp, vm::CellSlice& cs) const {
  pp.open("tuple");
  for (int i = 0; i < n; i++) {
    if (!pp.open(std::to_string(i)) ||
        !X.print_skip(pp, cs) ||
        !pp.close()) {
      return false;
    }
  }
  return pp.close();
}

bool CondT::print_skip(PrettyPrinter& pp, vm::CellSlice& cs) const {
  return (n > 0 ? X.print_skip(pp, cs) : (!n && pp.out("()")));
}

bool CondT::print_skip(JsonPrinter& pp, vm::CellSlice& cs) const {
  return (n > 0 ? X.print_skip(pp, cs) : (!n && pp.out("()")));
}

bool Int::print_skip(PrettyPrinter& pp, vm::CellSlice& cs) const {
  if (n <= 64) {
    long long value;
    return cs.fetch_int_to(n, value) && pp.out_int(value);
  } else {
    return pp.out_integer(cs.fetch_int256(n, true));
  }
}

bool Int::print_skip(JsonPrinter& pp, vm::CellSlice& cs) const {
  if (n <= 64) {
    long long value;
    return cs.fetch_int_to(n, value) && pp.out_int(value);
  } else {
    return pp.out_integer(cs.fetch_int256(n, true));
  }
}

bool UInt::print_skip(PrettyPrinter& pp, vm::CellSlice& cs) const {
  if (n <= 64) {
    unsigned long long value;
    return cs.fetch_uint_to(n, value) && pp.out_uint(value);
  } else {
    return pp.out_integer(cs.fetch_int256(n, false));
  }
}

bool UInt::print_skip(JsonPrinter& pp, vm::CellSlice& cs) const {
  if (n <= 64) {
    unsigned long long value;
    return cs.fetch_uint_to(n, value) && pp.out_uint(value);
  } else {
    return pp.out_integer(cs.fetch_int256(n, false));
  }
}

bool Bits::print_skip(PrettyPrinter& pp, vm::CellSlice& cs) const {
  if (cs.have(n)) {
    pp.os << 'x' << cs.fetch_bits(n).to_hex();
    return true;
  } else {
    return false;
  }
}

bool Bits::print_skip(JsonPrinter& pp, vm::CellSlice& cs) const {
  if (cs.have(n)) {
    return pp.out(cs.fetch_bits(n).to_hex());
  } else {
    return false;
  }
}

bool SnakeString::skip(vm::CellSlice& cs) const {
  vm::CellSlice current = cs;
  
  while (true) {
    // consume all bits in current cell
    if (!current.advance(current.size())) {
      return false;
    }
    if (current.size_refs() > 1) {
      return false;  // snake format can't have more than 1 ref
    }
    if (current.size_refs() == 0) {
      cs = current;
      return true;
    }
    // to next cell
    auto ref = current.fetch_ref();
    if (ref.is_null()) {
      return false;
    }
    if (!current.load(vm::NoVm{}, ref)) {
      return false;
    }
  }
}

bool SnakeString::validate_skip(int* ops, vm::CellSlice& cs, bool weak) const {
  if (ops && *ops <= 0) {
    return false;
  }
  vm::CellSlice current = cs;
  int cells_processed = 0;
  while (true) {
    if (ops) {
      (*ops)--;
      if (*ops < 0) {
        return false;
      }
    }
    cells_processed++;
    if (cells_processed > 1000) {
      return false;
    }
    if (!current.advance(current.size())) {
      return false;
    }
    if (current.size_refs() > 1) {
      return false;
    }
    if (current.size_refs() == 0) {
      cs = current;
      return true;
    }
    auto ref = current.fetch_ref();
    if (ref.is_null()) {
      return false;
    }
    if (!current.load(vm::NoVm{}, ref)) {
      return false;
    }
  }
}

td::Result<std::vector<unsigned char>> SnakeString::load_snake_binary(vm::CellSlice& cs) const {
  std::vector<unsigned char> data;
  vm::CellSlice current = cs;
  
  while (true) {
    unsigned bits_available = current.size();
    if (bits_available > 0) {
      while (bits_available >= 8) {
        int byte_val = current.fetch_octet();
        if (byte_val < 0) {
          return td::Status::Error("failed to fetch octet from snake cell");
        }
        data.push_back(static_cast<unsigned char>(byte_val));
        bits_available -= 8;
      }
      if (bits_available > 0) {
        unsigned long long remaining = current.fetch_ulong(bits_available);
        if (remaining == vm::CellSlice::fetch_ulong_eof) {
          return td::Status::Error("failed to fetch remaining bits from snake cell");
        }
        // shift remaining to form a byte
        unsigned char byte_val = static_cast<unsigned char>(remaining << (8 - bits_available));
        data.push_back(byte_val);
      }
    }
    if (current.size_refs() > 1) {
      return td::Status::Error("snake cell has more than one reference");
    }
    if (current.size_refs() == 0) {
      cs = current;
      return data;
    }
    auto ref = current.fetch_ref();
    if (ref.is_null()) {
      return td::Status::Error("snake cell reference is null");
    }
    if (!current.load(vm::NoVm{}, ref)) {
      return td::Status::Error("failed to load snake cell reference");
    }
  }
}

td::Result<std::string> SnakeString::load_snake_string(vm::CellSlice& cs) const {
  TRY_RESULT(binary_data, load_snake_binary(cs));
  return std::string(binary_data.begin(), binary_data.end());
}

bool SnakeString::print_skip(PrettyPrinter& pp, vm::CellSlice& cs) const {
  auto text_result = load_snake_string(cs);
  if (text_result.is_error()) {
    return pp.fail(text_result.error().message().str());
  }
  auto text = text_result.move_as_ok();
  pp.os << '"';
  for (char c : text) {
    if (c == '"') {
      pp.os << "\\\"";
    } else if (c == '\\') {
      pp.os << "\\\\";
    } else if (c == '\n') {
      pp.os << "\\n";
    } else if (c == '\r') {
      pp.os << "\\r";
    } else if (c == '\t') {
      pp.os << "\\t";
    } else if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) >= 127) {
      pp.os << "\\x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(static_cast<unsigned char>(c)) << std::dec;
    } else {
      pp.os << c;
    }
  }
  pp.os << '"';
  return true;
}

bool SnakeString::print_skip(JsonPrinter& pp, vm::CellSlice& cs) const {
  auto text_result = load_snake_string(cs);
  if (text_result.is_error()) {
    return pp.fail(text_result.error().message().str());
  }
  auto str = text_result.move_as_ok();
  if (!td::check_utf8(str)) {
    return pp.fail("invalid utf-8 string");
  }
  return pp.out(str);
}

bool TupleT::skip(vm::CellSlice& cs) const {
  int i = n;
  for (; i > 0; --i) {
    if (!X.skip(cs)) {
      break;
    }
  }
  return !i;
}

bool TupleT::validate_skip(int* ops, vm::CellSlice& cs, bool weak) const {
  int i = n;
  for (; i > 0; --i) {
    if (!X.validate_skip(ops, cs, weak)) {
      break;
    }
  }
  return !i;
}

bool TLB::validate_ref_internal(int* ops, Ref<vm::Cell> cell_ref, bool weak) const {
  if (ops) {
    if (*ops <= 0) {
      return false;
    }
    --*ops;
  }
  bool is_special;
  auto cs = load_cell_slice_special(std::move(cell_ref), is_special);
  if (cs.special_type() == vm::Cell::SpecialType::PrunnedBranch && weak) {
    return true;
  }
  if (always_special() != is_special) {
    return false;
  }
  return validate_skip(ops, cs, weak) && cs.empty_ext();
}

bool TLB::print_skip(PrettyPrinter& pp, vm::CellSlice& cs) const {
  pp.open("raw@");
  pp << *this << ' ';
  vm::CellSlice cs_copy{cs};
  int size_limit = pp.limit;
  if (!validate_skip(&size_limit, cs) || !cs_copy.cut_tail(cs)) {
    return pp.fail("invalid value");
  }
  pp.raw_nl();
  return (cs_copy.print_rec(pp.os, &pp.limit, pp.indent) && pp.mkindent() && pp.close()) ||
         pp.fail("raw value too long");
}

bool TLB::print_skip(tlb::JsonPrinter& pp, vm::CellSlice& cs) const {
  // print base64 encoded boc, so other programs can parse it
  vm::CellSlice cs_copy{cs};
  if (!validate_skip(nullptr, cs) || !cs_copy.cut_tail(cs)) {
    return pp.fail("invalid value");
  }
  vm::CellBuilder cb;
  cell_builder_add_slice(cb, cs_copy);
  Ref<vm::Cell> new_cell = cb.finalize();
  auto boc = vm::std_boc_serialize(new_cell);

  if (boc.is_ok()) {
    auto b64str = td::base64_encode(boc.move_as_ok().as_slice());
    return pp.out(b64str);
  } else {
    return pp.fail("failed to serialize cell");
  }
}

bool TLB::print_special(PrettyPrinter& pp, vm::CellSlice& cs) const {
  pp.open("raw@");
  pp << *this << ' ';
  pp.raw_nl();
  return (cs.print_rec(pp.os, &pp.limit, pp.indent) && pp.mkindent() && pp.close()) || pp.fail("raw value too long");
}

bool TLB::print_special(JsonPrinter& pp, vm::CellSlice& cs) const {
  return print_skip(pp, cs);
}

bool TLB::print_ref(PrettyPrinter& pp, Ref<vm::Cell> cell_ref) const {
  if (cell_ref.is_null()) {
    return pp.fail("null cell reference");
  }
  if (!pp.register_recursive_call()) {
    return pp.fail("too many recursive calls while printing a TL-B value");
  }
  bool is_special;
  auto cs = load_cell_slice_special(std::move(cell_ref), is_special);
  if (is_special) {
    return print_special(pp, cs);
  } else {
    return print_skip(pp, cs) && (cs.empty_ext() || pp.fail("extra data in cell"));
  }
}

bool TLB::print_ref(JsonPrinter& pp, Ref<vm::Cell> cell_ref) const {
  if (cell_ref.is_null()) {
    return pp.fail("null cell reference");
  }
  if (!pp.register_recursive_call()) {
    return pp.fail("too many recursive calls while printing a TL-B value");
  }
  bool is_special;
  auto cs = load_cell_slice_special(std::move(cell_ref), is_special);
  if (is_special) {
    return print_special(pp, cs);
  } else {
    return print_skip(pp, cs);
  }
}

bool TLB::print_skip(std::ostream& os, vm::CellSlice& cs, int indent, int rec_limit) const {
  PrettyPrinter pp{os, indent};
  pp.set_limit(rec_limit);
  return pp.fail_unless(print_skip(pp, cs));
}

bool TLB::print(std::ostream& os, const vm::CellSlice& cs, int indent, int rec_limit) const {
  PrettyPrinter pp{os, indent};
  pp.set_limit(rec_limit);
  return pp.fail_unless(print(pp, cs));
}

bool TLB::print_ref(std::ostream& os, Ref<vm::Cell> cell_ref, int indent, int rec_limit) const {
  PrettyPrinter pp{os, indent};
  pp.set_limit(rec_limit);
  return pp.fail_unless(print_ref(pp, std::move(cell_ref)));
}

bool TLB::print_ref(td::StringBuilder& sb, Ref<vm::Cell> cell_ref, int indent, int rec_limit) const {
  std::ostringstream ss;
  auto result = print_ref(ss, std::move(cell_ref), indent, rec_limit);
  sb << ss.str();
  return result;
}

std::string TLB::as_string_skip(vm::CellSlice& cs, int indent) const {
  std::ostringstream os;
  print_skip(os, cs, indent);
  return os.str();
}

std::string TLB::as_string(const vm::CellSlice& cs, int indent) const {
  std::ostringstream os;
  print(os, cs, indent);
  return os.str();
}

std::string TLB::as_string_ref(Ref<vm::Cell> cell_ref, int indent) const {
  std::ostringstream os;
  print_ref(os, std::move(cell_ref), indent);
  return os.str();
}

PrettyPrinter::~PrettyPrinter() {
  if (failed || level) {
    if (nl_used) {
      nl(-2 * level);
    }
    os << "PRINTING FAILED";
    while (level > 0) {
      os << ')';
      --level;
    }
  }
  if (nl_used) {
    os << std::endl;
  }
}

bool PrettyPrinter::fail(std::string msg) {
  os << "<FATAL: " << msg << ">" << std::endl;
  failed = true;
  return false;
}

bool PrettyPrinter::mkindent(int delta) {
  indent += delta;
  for (int i = 0; i < indent; i++) {
    os << ' ';
  }
  nl_used = true;
  return true;
}

bool PrettyPrinter::nl(int delta) {
  os << std::endl;
  return mkindent(delta);
}
bool PrettyPrinter::raw_nl(int delta) {
  os << std::endl;
  indent += delta;
  nl_used = true;
  return true;
}

bool PrettyPrinter::open(std::string msg) {
  os << "(" << msg;
  indent += 2;
  level++;
  return true;
}

bool PrettyPrinter::close() {
  return close("");
}

bool PrettyPrinter::close(std::string msg) {
  if (level <= 0) {
    return fail("cannot close scope");
  }
  indent -= 2;
  --level;
  os << msg << ")";
  return true;
}

bool PrettyPrinter::mode_nl() {
  if (mode & 1) {
    return nl();
  } else {
    os << ' ';
    return true;
  }
}

bool PrettyPrinter::field(std::string name) {
  mode_nl();
  os << name << ':';
  return true;
}

bool PrettyPrinter::field() {
  mode_nl();
  return true;
}

bool PrettyPrinter::field_int(long long x, std::string name) {
  os << ' ' << name << ':' << x;
  return true;
}

bool PrettyPrinter::field_int(long long x) {
  os << ' ' << x;
  return true;
}

bool PrettyPrinter::field_uint(unsigned long long x, std::string name) {
  os << ' ' << name << ':' << x;
  return true;
}

bool PrettyPrinter::field_uint(unsigned long long x) {
  os << ' ' << x;
  return true;
}

bool PrettyPrinter::fetch_bits_field(vm::CellSlice& cs, int n) {
  os << " x";
  return cs.have(n) && out(cs.fetch_bits(n).to_hex());
}

bool PrettyPrinter::fetch_bits_field(vm::CellSlice& cs, int n, std::string name) {
  os << ' ' << name << ":x";
  return cs.have(n) && out(cs.fetch_bits(n).to_hex());
}

bool PrettyPrinter::fetch_int_field(vm::CellSlice& cs, int n) {
  return cs.have(n) && field_int(cs.fetch_long(n));
}

bool PrettyPrinter::fetch_int_field(vm::CellSlice& cs, int n, std::string name) {
  return cs.have(n) && field_int(cs.fetch_long(n), name);
}

bool PrettyPrinter::fetch_uint_field(vm::CellSlice& cs, int n) {
  return cs.have(n) && field_uint(cs.fetch_ulong(n));
}

bool PrettyPrinter::fetch_uint_field(vm::CellSlice& cs, int n, std::string name) {
  return cs.have(n) && field_uint(cs.fetch_ulong(n), name);
}

bool PrettyPrinter::fetch_int256_field(vm::CellSlice& cs, int n) {
  os << ' ';
  return out_integer(cs.fetch_int256(n, true));
}

bool PrettyPrinter::fetch_int256_field(vm::CellSlice& cs, int n, std::string name) {
  os << ' ' << name << ':';
  return out_integer(cs.fetch_int256(n, true));
}

bool PrettyPrinter::fetch_uint256_field(vm::CellSlice& cs, int n) {
  os << ' ';
  return out_integer(cs.fetch_int256(n, false));
}

bool PrettyPrinter::fetch_uint256_field(vm::CellSlice& cs, int n, std::string name) {
  os << ' ' << name << ':';
  return out_integer(cs.fetch_int256(n, false));
}

bool PrettyPrinter::fetch_bool_field(vm::CellSlice& cs) {
  os << ' ';
  return cs.have(1) && out(cs.fetch_ulong(1) ? "true" : "false");
}

bool PrettyPrinter::fetch_bool_field(vm::CellSlice& cs, std::string name) {
  os << ' ' << name << ':';
  return cs.have(1) && out(cs.fetch_ulong(1) ? "true" : "false");
}

}  // namespace tlb

namespace tlb {

bool TypenameLookup::register_types(typename TypenameLookup::register_func_t func) {
  return func([this](const char* name, const TLB* tp) { return register_type(name, tp); });
}

bool TypenameLookup::register_type(const char* name, const TLB* tp) {
  if (!name || !tp) {
    return false;
  }
  auto res = types.emplace(name, tp);
  return res.second;
}

const TLB* TypenameLookup::lookup(std::string str) const {
  auto it = types.find(str);
  return it != types.end() ? it->second : nullptr;
}

const TLB* TypenameLookup::lookup(td::Slice str) const {
  auto it = std::lower_bound(types.begin(), types.end(), str,
                             [](const auto& x, const auto& y) { return td::Slice(x.first) < y; });
  return it != types.end() && td::Slice(it->first) == str ? it->second : nullptr;
}

std::string JsonPrinter::escape_string(const std::string& str) {
  std::string result;
  result.reserve(str.size() + 10);

  for (char c : str) {
    switch (c) {
      case '"': result += "\\\""; break;
      case '\\': result += "\\\\"; break;
      case '\n': result += "\\n"; break;
      case '\r': result += "\\r"; break;
      case '\t': result += "\\t"; break;
      case '\b': result += "\\b"; break;
      case '\f': result += "\\f"; break;
      default:
        if (static_cast<unsigned char>(c) < 32 || c == 127) {
          // escaping control chars and DEL
          char buf[7];
          snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
          result += buf;
        } else {
          result += c;
        }
    }
  }
  return result;
}

bool JsonPrinter::open(std::string msg) {
  // if (buffer().empty() && level_ == 0) {
  //   buffer() += "{";
  // }
  if (msg.empty())
    buffer() += "{";
  else {
    // if (after_semicolon_) {
    buffer() += "{\"@type\":\"" + escape_string(msg) + "\"";
    // }
    // else {
    //   if (!first_field_) buffer() += ",";
    //   buffer() += "\"" + escape_string(msg) + "\":" + "{\"@type\":\"" + escape_string(msg) + "\"";
    // }
  }
  level_++;
  first_field_ = false;
  if (msg.empty()) first_field_ = true;
  after_semicolon_ = false;
  return true;
}

bool JsonPrinter::close() {
  return close("");
}

bool JsonPrinter::close(std::string msg) {
  level_--;
  if (level_ < 0) {
    failed_ = true;
    return false;
  }

  buffer() += "}";
  first_field_ = false;
  after_semicolon_ = false;
  // if (level_ == 0) { // last ever
  //   buffer() += "}";
  // }
  return true;
}

bool JsonPrinter::field(std::string name) {
  if (!first_field_) buffer() += ",";
  buffer() += "\"" + escape_string(name) + "\":";
  first_field_ = false;
  after_semicolon_ = true;
  return true;
}

bool JsonPrinter::field() {
  // return true;
  return field("");
}

bool JsonPrinter::field_int(long long value) {
  after_semicolon_ = false;
  return field_int(value, "");
}

bool JsonPrinter::field_int(long long value, std::string name) {
  after_semicolon_ = false;
  if (!name.empty()) field(name);
  buffer() += "\"" + std::to_string(value) + "\"";
  return true;
}

bool JsonPrinter::field_uint(unsigned long long value) {
  after_semicolon_ = false;
  return field_uint(value, "");
}

bool JsonPrinter::field_uint(unsigned long long value, std::string name) {
  after_semicolon_ = false;
  if (!name.empty()) field(name);
  buffer() += "\"" + std::to_string(value) + "\"";
  return true;
}

bool JsonPrinter::fetch_bits_field(vm::CellSlice& cs, int n) {
  if (!after_semicolon_) field("");
  after_semicolon_ = false;
  if (!cs.have(n)) return false;
  auto bits = cs.fetch_bits(n);
  buffer() += "\"" + bits.to_hex() + "\"";
  return true;
}

bool JsonPrinter::fetch_bits_field(vm::CellSlice& cs, int n, std::string name) {
  after_semicolon_ = false;
  if (!name.empty()) field(name);
  return fetch_bits_field(cs, n);
}

bool JsonPrinter::fetch_int_field(vm::CellSlice& cs, int n) {
  if (!after_semicolon_) field("");
  after_semicolon_ = false;
  if (!cs.have(n)) return false;
  long long value = cs.fetch_long(n);
  return field_int(value);
}

bool JsonPrinter::fetch_int_field(vm::CellSlice& cs, int n, std::string name) {
  after_semicolon_ = false;
  if (!name.empty()) field(name);
  return fetch_int_field(cs, n);
}

bool JsonPrinter::fetch_uint_field(vm::CellSlice& cs, int n) {
  if (!after_semicolon_) field("");
  after_semicolon_ = false;
  if (!cs.have(n)) return false;
  unsigned long long value = cs.fetch_ulong(n);
  return field_uint(value);
}

bool JsonPrinter::fetch_uint_field(vm::CellSlice& cs, int n, std::string name) {
  after_semicolon_ = false;
  if (!name.empty()) field(name);
  return fetch_uint_field(cs, n);
}

bool JsonPrinter::fetch_int256_field(vm::CellSlice& cs, int n) {
  if (!after_semicolon_) field("");
  after_semicolon_ = false;
  if (!cs.have(n)) return false;
  auto value = cs.prefetch_int256(n, true);
  if (value.not_null()) {
    std::ostringstream oss;
    oss << value;
    buffer() += "\"" + oss.str() + "\"";
    cs.fetch_int256(n, true);
    return true;
  }
  return false;
}

bool JsonPrinter::fetch_int256_field(vm::CellSlice& cs, int n, std::string name) {
  after_semicolon_ = false;
  if (!name.empty()) field(name);
  return fetch_int256_field(cs, n);
}

bool JsonPrinter::fetch_uint256_field(vm::CellSlice& cs, int n) {
  if (!after_semicolon_) field("");
  after_semicolon_ = false;
  if (!cs.have(n)) return false;
  auto value = cs.prefetch_int256(n, false);
  if (value.not_null()) {
    std::ostringstream oss;
    oss << value;
    buffer() += "\"" + oss.str() + "\"";
    cs.fetch_int256(n, false);
    return true;
  }
  return false;
}

bool JsonPrinter::fetch_uint256_field(vm::CellSlice& cs, int n, std::string name) {
  after_semicolon_ = false;
  if (!name.empty()) field(name);
  return fetch_uint256_field(cs, n);
}

bool JsonPrinter::fetch_bool_field(vm::CellSlice& cs) {
  if (!after_semicolon_) field("");
  after_semicolon_ = false;
  return cs.have(1) && write_raw(cs.fetch_ulong(1) ? "true" : "false");
}

bool JsonPrinter::fetch_bool_field(vm::CellSlice& cs, std::string name) {
  after_semicolon_ = false;
  if (!name.empty()) field(name);
  return fetch_bool_field(cs);
}

bool JsonPrinter::out(std::string str) {
  after_semicolon_ = false;
  buffer() += "\"" + escape_string(str) + "\"";
  return true;
}

bool JsonPrinter::out_int(long long value) {
  after_semicolon_ = false;
  buffer() += "\"" + std::to_string(value) + "\"";
  return true;
}

bool JsonPrinter::out_uint(unsigned long long value) {
  after_semicolon_ = false;
  buffer() += "\"" + std::to_string(value) + "\"";
  return true;
}

bool JsonPrinter::out_integer(td::RefInt256 value) {
  after_semicolon_ = false;
  if (value.not_null()) {
    std::ostringstream oss;
    oss << value;
    buffer() += "\"" + oss.str() + "\"";
    return true;
  }
  return false;
}

bool JsonPrinter::cons(std::string str) {
  return open(str) && close();
}

bool JsonPrinter::register_recursive_call() {
  return limit_--;
}

void JsonPrinter::set_limit(int new_limit) {
  if (new_limit > 0) {
    limit_ = new_limit;
  }
}

bool JsonPrinter::fail(std::string msg) {
  buffer() += "\"<FATAL: " + escape_string(msg) + ">\"";
  level_++;
  while (level_ > 0) {
    buffer() += "}";
    level_--;
  }
  failed_ = true;
  return false;
}


bool JsonPrinter::fail_unless(bool res) {
  if (!res) {
    failed_ = true;
  }
  return res;
}

bool JsonPrinter::write_raw(const std::string& json) {
  buffer() += json;
  return true;
}

}  // namespace tlb
