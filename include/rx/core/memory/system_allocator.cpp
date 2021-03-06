#include "rx/core/memory/system_allocator.h"
#include "rx/core/memory/electric_fence_allocator.h"
#include "rx/core/memory/heap_allocator.h"

namespace rx::memory {

system_allocator::system_allocator()
#if defined(RX_ESAN)
  : m_stats_allocator{electric_fence_allocator::instance()}
#else
  : m_stats_allocator{heap_allocator::instance()}
#endif
{
}

rx_byte* system_allocator::allocate(rx_size _size) {
  return m_stats_allocator.allocate(_size);
}

rx_byte* system_allocator::reallocate(void* _data, rx_size _size) {
  return m_stats_allocator.reallocate(_data, _size);
}

void system_allocator::deallocate(void* _data) {
  return m_stats_allocator.deallocate(_data);
}

global<system_allocator> system_allocator::s_instance{"system", "allocator"};

} // namespace rx::memory
