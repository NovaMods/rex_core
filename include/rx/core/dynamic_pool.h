#ifndef RX_CORE_DYNAMIC_POOL_H
#define RX_CORE_DYNAMIC_POOL_H
#include "rx/core/static_pool.h"
#include "rx/core/vector.h"
#include "rx/core/ptr.h"

#include "rx/core/hints/empty_bases.h"

namespace rx {

struct RX_HINT_EMPTY_BASES dynamic_pool
  : concepts::no_copy
{
  constexpr dynamic_pool(memory::allocator& _allocator, rx_size _object_size, rx_size _objects_per_pool);
  constexpr dynamic_pool(rx_size _object_size, rx_size _per_pool);
  dynamic_pool(dynamic_pool&& pool_);

  dynamic_pool& operator=(dynamic_pool&& pool_);
  rx_byte* operator[](rx_size _index) const;

  rx_size allocate();
  void deallocate(rx_size _index);

  template<typename T, typename... Ts>
  T* create(Ts&&... _arguments);

  template<typename T>
  void destroy(T* _data);

  constexpr memory::allocator& allocator() const;

  rx_size object_size() const;
  rx_size size() const;

  rx_byte* data_of(rx_size _index) const;
  rx_size index_of(const rx_byte* _data) const;

private:
  [[nodiscard]] bool add_pool();
  rx_size pool_index_of(const rx_byte* _data) const;

  ref<memory::allocator> m_allocator;
  rx_size m_object_size;
  rx_size m_objects_per_pool;
  vector<ptr<static_pool>> m_pools;
};

inline constexpr dynamic_pool::dynamic_pool(memory::allocator& _allocator, rx_size _object_size, rx_size _objects_per_pool)
  : m_allocator{_allocator}
  , m_object_size{_object_size}
  , m_objects_per_pool{_objects_per_pool}
  , m_pools{allocator()}
{
}

inline constexpr dynamic_pool::dynamic_pool(rx_size _object_size, rx_size _per_pool)
  : dynamic_pool{memory::system_allocator::instance(), _object_size, _per_pool}
{
}

RX_HINT_FORCE_INLINE rx_byte* dynamic_pool::operator[](rx_size _index) const {
  return data_of(_index);
}

template<typename T, typename... Ts>
inline T* dynamic_pool::create(Ts&&... _arguments) {
  const rx_size pools = m_pools.size();
  for (rx_size i = 0; i < pools; i++) {
    auto& pool = m_pools[i];
    if (pool->can_allocate()) {
      return pool->create<T>(utility::forward<Ts>(_arguments)...);
    }
  }

  if (add_pool()) {
    return create<T>(utility::forward<Ts>(_arguments)...);
  }

  return nullptr;
}

template<typename T>
void dynamic_pool::destroy(T* _data) {
  const rx_size index = pool_index_of(reinterpret_cast<const rx_byte*>(_data));
  if (index == -1_z) {
    return;
  }

  // Fetch the static pool with the given index, then destroy the data on
  // that pool, as it own's it.
  auto& pool = m_pools[index];
  pool->destroy<T>(_data);

  // When the pool is empty and it's the last pool in the list, to reduce
  // memory, remove it from |m_pools|.
  if (pool->is_empty() && pool == m_pools.last()) {
    m_pools.pop_back();
  }
}

RX_HINT_FORCE_INLINE constexpr memory::allocator& dynamic_pool::allocator() const {
  return m_allocator;
}

RX_HINT_FORCE_INLINE rx_size dynamic_pool::object_size() const {
  return m_object_size;
}

RX_HINT_FORCE_INLINE rx_size dynamic_pool::size() const {
  return m_pools.size();
}

} // namespace rx

#endif // RX_CORE_DYNAMIC_POOL_H
