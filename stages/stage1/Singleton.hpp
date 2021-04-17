//
// Created by miche on 10/04/2021.
//

#ifndef SILKWORM_SINGLETON_HPP
#define SILKWORM_SINGLETON_HPP

#include <memory>

template <class T>
class Singleton {
    static std::unique_ptr<T> instance_;
  public:
    static void instance(std::unique_ptr<T> instance) {instance_ = instance;}
    static T& instance() {return *instance_;}
};

#endif  // SILKWORM_SINGLETON_HPP
