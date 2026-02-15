#ifndef _AMQP_ROUTER_HPP_
#define _AMQP_ROUTER_HPP_
#include <amqpcpp.h>

#include "limb-app.h"
#include "thread-pool/thread-pool.hpp"

#include "utils/status.h"

const char END_MESSAGE[] = "end";

liret setRoutes(limb::AppBase *application, limb::tp::ThreadPool &tp, AMQP::Connection &conn, AMQP::Channel &ch);

#endif // _AMQP_ROUTER_HPP_