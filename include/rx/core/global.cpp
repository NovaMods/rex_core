#include <string.h> // strcmp
#include <stdlib.h> // malloc, free

#include "rx/core/global.h"
#include "rx/core/log.h"

#include "rx/core/concurrency/scope_lock.h"
#include "rx/core/concurrency/spin_lock.h"

namespace rx {

RX_LOG("global", logger);

static concurrency::spin_lock g_lock;

// global_node
void global_node::init_global() {
  if (!(m_flags & k_enabled)) {
    return;
  }

  RX_ASSERT(!(m_flags & k_initialized), "already initialized");
  logger(log::level::k_verbose, "%p init: %s/%s", this, m_group, m_name);

  m_storage_dispatch(storage_mode::k_init_global, data(), m_argument_store);

  m_flags |= k_initialized;
}

void global_node::fini_global() {
  if (!(m_flags & k_enabled)) {
    return;
  }

  RX_ASSERT(m_flags & k_initialized, "not initialized");
  logger(log::level::k_verbose, "%p fini: %s/%s", this, m_group, m_name);

  m_shared->finalizer(data());
  if (m_flags & k_arguments) {
    m_storage_dispatch(storage_mode::k_fini_arguments, data(), m_argument_store);
    deallocate_arguments(m_argument_store);
  }
  m_flags &= ~k_initialized;
}

void global_node::init() {
  RX_ASSERT(!(m_flags & k_initialized), "already initialized");

  m_storage_dispatch(storage_mode::k_init_global, data(), m_argument_store);

  m_flags &= ~k_enabled;
  m_flags |= k_initialized;
}

void global_node::fini() {
  RX_ASSERT(m_flags & k_initialized, "not initialized");

  m_shared->finalizer(data());
  if (m_flags & k_arguments) {
    m_storage_dispatch(storage_mode::k_fini_arguments, data(), m_argument_store);
    deallocate_arguments(m_argument_store);
  }
  m_flags &= ~k_enabled;
  m_flags |= k_initialized;
}

rx_byte* global_node::allocate_arguments(rx_size _size) {
  return reinterpret_cast<rx_byte*>(malloc(_size));
}

void global_node::deallocate_arguments(rx_byte* _arguments) {
  free(_arguments);
}

// global_group
global_node* global_group::find(const char* _name) {
  for (auto node = m_list.enumerate_head(&global_node::m_grouped); node; node.next()) {
    if (!strcmp(node->name(), _name)) {
      return node.data();
    }
  }
  return nullptr;
}

void global_group::init() {
  for (auto node = m_list.enumerate_head(&global_node::m_grouped); node; node.next()) {
    node->init();
  }
}

void global_group::fini() {
  for (auto node = m_list.enumerate_tail(&global_node::m_grouped); node; node.prev()) {
    node->fini();
  }
}

void global_group::init_global() {
  for (auto node = m_list.enumerate_head(&global_node::m_grouped); node; node.next()) {
    node->init_global();
  }
}

void global_group::fini_global() {
  for (auto node = m_list.enumerate_tail(&global_node::m_grouped); node; node.prev()) {
    node->fini_global();
  }
}

// globals
global_group* globals::find(const char* _name) {
  for (auto group = s_group_list.enumerate_head(&global_group::m_link); group; group.next()) {
    if (!strcmp(group->name(), _name)) {
      return group.data();
    }
  }
  return nullptr;
}

void globals::link() {
  // Link ungrouped globals from |s_node_list| managed by |global_node::m_ungrouped|
  // with the appropriate group given by |global_group::m_list|, which is managed
  // by |global_node::m_grouped| when the given global shares the same group
  // name as the group.
  concurrency::scope_lock lock{g_lock};
  for (auto node = s_node_list.enumerate_head(&global_node::m_ungrouped); node; node.next()) {
    bool unlinked = true;
    for (auto group = s_group_list.enumerate_head(&global_group::m_link); group; group.next()) {
      if (!strcmp(node->m_group, group->name())) {
        group->m_list.push(&node->m_grouped);
        unlinked = false;
        break;
      }
    }

    if (unlinked) {
      // NOTE(dweiler): If you've hit this code-enforced crash it means there
      // exists an rx::global<T> that is associated with a group by name which
      // doesn't exist. This can be caused by misnaming the group in the
      // global's constructor, or because the rx::global_group with that name
      // doesn't exist in any translation unit.
      *reinterpret_cast<volatile int*>(0) = 0;
    }
  }
}

void globals::init() {
  for (auto group = s_group_list.enumerate_head(&global_group::m_link); group; group.next()) {
    group->init_global();
  }
}

void globals::fini() {
  for (auto group = s_group_list.enumerate_tail(&global_group::m_link); group; group.prev()) {
    group->fini_global();
  }
}

void globals::link(global_node* _node) {
  concurrency::scope_lock lock{g_lock};
  s_node_list.push(&_node->m_ungrouped);
}

void globals::link(global_group* _group) {
  concurrency::scope_lock lock{g_lock};
  s_group_list.push(&_group->m_link);
}

static global_group g_group_system{"system"};

} // namespace rx