#ifndef _MONGO_CLIENT_HPP_
#define _MONGO_CLIENT_HPP_
#include "utils/status.h"
#include <mongoc/mongoc.h>

#define MONGO_TRANSFER_CHUNK 4096

namespace limb {

class MongoService {
public:
  MongoService(const char *dbname, mongoc_client_pool_t *cpool);
  static int mongoTest();
  liret getImageById(const bson_oid_t *oid, unsigned char **filedata, ssize_t *filesize);
  liret updateImageById(const bson_oid_t *oid, unsigned char *filedata, ssize_t filesize);

private:
  mongoc_client_pool_t *cpool;
  const char *dbname;
};

// Provides access to mongo services
extern MongoService *mongoService;

} // namespace limb
#endif // _MONGO_CLIENT_HPP_