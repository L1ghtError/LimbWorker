#ifndef _AMQP_ROUTER_HPP_
#define _AMQP_ROUTER_HPP_
#include "utils/status.h"
#include <amqpcpp.h>

liret setRoutes(AMQP::Connection &conn, AMQP::Channel &ch);

#endif // _AMQP_ROUTER_HPP_