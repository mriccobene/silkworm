/*
   Copyright 2021 The Silkworm Authors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include <mutex>
#include <map>

// thread_safe_map
//
// As std::map, but:
// 1. has lock policy
// 2. has additional methods for common patterns in a concurrent context
// 
// todo: choose a better alternative, for example oneTBB::ConcurrentHashMap

template<class KEY, class VAL, class M = std::recursive_mutex, class Cmp = std::less<KEY> >   // oppure M = std::mutex oppure M = null_mutex ...
class ThreadSafeMap {
public:
    typedef typename std::map<KEY, VAL, Cmp> container_type;
    typedef KEY key_type;
    typedef VAL mapped_type;
    typedef typename container_type::value_type value_type;  // pair()
    typedef typename container_type::size_type size_type;
    typedef typename container_type::iterator iterator;
    typedef M mutex_type;
    typedef typename std::unique_lock<mutex_type> guard_type;

    ThreadSafeMap() {}

    virtual ~ThreadSafeMap() {}

    mapped_type& operator[](const key_type& key) {
        guard_type guard(mutex_);
        return container_[key];
    }

    template<class... Args>
    std::pair<iterator, bool> emplace(Args&& ... args) {
        guard_type guard(mutex_);
        return container_.emplace(std::forward<Args>(args)...);
    }

    // As operator[] but:
    // 1) copies the value (!)
    // 2) doesn't insert a new pair if the key is not present
    mapped_type value_of(const key_type& key, mapped_type default_value = VAL()) {
        guard_type guard(mutex_);
        iterator i = container_.find(key);
        if (i != container_.end()) return (*i).second;
        return default_value;
    }

    // As operator[] but:
    // 1) doesn't insert a new pair if the key is not present
    mapped_type& ref_value_of(const key_type& key, mapped_type& default_value) {
        guard_type guard(mutex_);
        iterator i = container_.find(key);
        if (i != container_.end()) return (*i).second;
        return default_value;
    }

    // As the preceding method but has a constant as default
    mapped_type& ref_value_of(const key_type& key) {
        guard_type guard(mutex_);
        iterator i = container_.find(key);
        return (*i).second;
    }

    bool contain(const key_type& key) {
        guard_type guard(mutex_);
        iterator i = container_.find(key);
        if (i != container_.end()) return true;
        return false;
    }

    iterator find(const key_type& key) {
        guard_type guard(mutex_);
        return container_.find(key);
    }

    size_type erase(const key_type& key) {
        guard_type guard(mutex_);
        return container_.erase(key);
    }

    iterator erase(iterator i) {
        guard_type guard(mutex_);
        return container_.erase(i);
    }

    void clear() {
        guard_type guard(mutex_);
        container_.clear();
    }

    size_type size() {
        guard_type guard(mutex_);
        return container_.size();
    }

    iterator begin() {
        guard_type guard(mutex_);
        return container_.begin();
    }

    iterator end() {
        guard_type guard(mutex_);
        return container_.end();
    }

    void lock() { mutex_.lock(); }
    void unlock() { mutex_.unlock(); }

    // Need explicit lock/unlock
    container_type& container() { return container_; }

private:
    mutex_type mutex_;
    container_type container_;
};



