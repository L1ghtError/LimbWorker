#ifndef _MONGO_CLIENT_HPP_
#define _MONGO_CLIENT_HPP_

#include <mongoc/mongoc.h>

#include "media-repository.hpp"
#include "utils/status.h"

#define MONGO_TRANSFER_CHUNK 4096

namespace limb {

class MongoClient : public MediaRepository {
public:
  MongoClient(const char *dbUri, const char *dbName);
  ~MongoClient();

  liret init(bson_error_t *error);
  void destroyMongoClient();

  static int mongoTest();
  liret ping(bson_error_t *error) const;

  liret getImageById(const char *id, size_t size, unsigned char **filedata, size_t *filesize) const override;
  liret updateImageById(const char *id, size_t size, unsigned char *filedata, size_t filesize) const override;

private:
  mongoc_uri_t *m_uri;
  mongoc_client_pool_t *m_cPool;

  const char *m_dbName;
  const char *m_dbUri;
};

} // namespace limb
#endif // _MONGO_CLIENT_HPP_