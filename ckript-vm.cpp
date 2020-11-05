#include "ckript-vm.hpp"

#include <cassert>

bool Value::is_lvalue() {
  return reference_name.size() != 0;
}

bool Variable::is_reference() const {
  return var_reference != -1;
}

bool Variable::is_allocated() const {
  return heap_reference != -1;
}

Chunk &Heap::allocate() {
  std::int64_t index = 0;
  // try to find a free chunk
  for (auto &chunk : chunks) {
    if (!chunk.used) {
      chunk.used = true;
      chunk.data = new Variable;
      chunk.data->heap_reference = index;
      return chunk;
    }
    index++;
  }
  // add a new chunk
  chunks.push_back(Chunk());
  Chunk &chunk_ref = chunks.back();
  chunk_ref.used = true;
  chunk_ref.data = new Variable;
  chunk_ref.data->heap_reference = chunks.size();
  return chunk_ref;
}

void Heap::free(Variable *var) {
  // check for all possible errors
  assert(var->is_allocated());
  assert(var->heap_reference < chunks.size());
  Chunk &chunk = chunks.at(var->heap_reference);
  assert(chunk.used == true);
  assert(chunk.data != nullptr);

  delete chunk.data;
  chunk.data = nullptr;
  chunk.used = false;
  var->heap_reference = -1;
  // shrink the heap if possible
  while (chunks.size() && !chunks.back().used) {
    chunks.pop_back();
  }
}