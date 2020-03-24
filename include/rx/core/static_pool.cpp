#include "rx/core/static_pool.h"

#include "rx/core/utility/exchange.h"

namespace rx {

static_pool::static_pool(memory::allocator* _allocator, rx_size _object_size, rx_size _capacity)
  : m_allocator{_allocator}
  , m_object_size{memory::allocator::round_to_alignment(_object_size)}
  , m_capacity{_capacity}
  , m_data{_allocator->allocate(m_object_size * m_capacity)}
  , m_bitset{m_allocator, m_capacity}
{
}

static_pool::static_pool(static_pool&& pool_)
  : m_allocator{utility::exchange(pool_.m_allocator, nullptr)}
  , m_object_size{utility::exchange(pool_.m_object_size, 0)}
  , m_capacity{utility::exchange(pool_.m_capacity, 0)}
  , m_data{utility::exchange(pool_.m_data, nullptr)}
  , m_bitset{utility::move(pool_.m_bitset)}
{
}

static_pool& static_pool::operator=(static_pool&& pool_) {
  RX_ASSERT(&pool_ != this, "self assignment");

  m_allocator = utility::exchange(pool_.m_allocator, nullptr);
  m_object_size = utility::exchange(pool_.m_object_size, 0);
  m_capacity = utility::exchange(pool_.m_capacity, 0);
  m_data = utility::exchange(pool_.m_data, nullptr);
  m_bitset = utility::move(pool_.m_bitset);

  return *this;
}

rx_size static_pool::allocate() {
  const rx_size index{m_bitset.find_first_unset()};
  if (RX_HINT_UNLIKELY(index == -1_z)) {
    return -1_z;
  }

  m_bitset.set(index);
  return index;
}

void static_pool::deallocate(rx_size _index) {
  RX_ASSERT(m_bitset.test(_index), "unallocated");
  m_bitset.clear(_index);
}

} // namespace rx
