#include "ckript-vm.hpp"

Chunk &Heap::allocate() {
  std::uint64_t index;
  for (auto &chunk : chunks) {
    if (!chunk.used) {
      chunk.used = true;
      chunk.data = new Variable;
      chunk.data->heap_reference = index;
      return chunk;
    }
    index++;
  }
  chunks.push_back(Chunk()); // add a new chunk
  Chunk &chunk_ref = chunks.back();
  chunk_ref.used = true;
  chunk_ref.data = new Variable;
  chunk_ref.data->heap_reference = chunks.size();
  return chunk_ref;
}

void Heap::free(Variable *var) {
  chunks.at(var->heap_reference).used = false;
  delete chunks.at(var->heap_reference).data;
  var->heap_reference = -1;
  // shrink the heap if possible
  while (chunks.size() && !chunks.back().used) {
    chunks.pop_back();
  }
}