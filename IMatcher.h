#ifndef IMATCHER_H
#define IMATCHER_H

#include "Order.h"
#include <memory>

using std::shared_ptr;

class IMatcher {
public:
    virtual void matchOrder(std::shared_ptr<Order> order) = 0;
    virtual ~IMatcher() = default;
};

#endif // IMATCHER_H
