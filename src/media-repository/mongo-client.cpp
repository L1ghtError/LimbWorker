#include "media-repository/mongo-client.hpp"

#include <stdio.h>
#include <stdlib.h>

#define MONGO_TRANSFER_CHUNK 4096
#define MONGO_TIMEOUT_MS 5000

namespace limb {
  class MongoInitializer {
  public:
    MongoInitializer() noexcept { mongoc_init(); }
    ~MongoInitializer() noexcept { mongoc_cleanup(); }

    MongoInitializer(const MongoInitializer &) = delete;
    MongoInitializer &operator=(const MongoInitializer &) = delete;
    MongoInitializer(MongoInitializer &&) = delete;
    MongoInitializer &operator=(MongoInitializer &&) = delete;
  };

  static MongoInitializer s_mongo_init;

void MongoClient::destroyMongoClient() {
  if (m_cPool != nullptr) {
    mongoc_client_pool_destroy(m_cPool);
  }
  if (m_uri != nullptr) {
    mongoc_uri_destroy(m_uri);
  }
}

MongoClient::~MongoClient() { destroyMongoClient(); }

MongoClient::MongoClient(const char *dbUri, const char *dbName)
    : m_dbName(dbName), m_dbUri(dbUri), m_uri(nullptr), m_cPool(nullptr) {}

liret MongoClient::init(bson_error_t *error) {
  this->m_uri = mongoc_uri_new_with_error(this->m_dbUri, error);
  if (this->m_uri == nullptr) {
    return liret::kInvalidInput;
  }

  mongoc_uri_set_option_as_int32(this->m_uri, MONGOC_URI_SERVERSELECTIONTIMEOUTMS, MONGO_TIMEOUT_MS);

  this->m_cPool = mongoc_client_pool_new(this->m_uri);
  if (this->m_cPool == nullptr) {
    return liret::kUninitialized;
  }

  if (liret::kOk != ping(error)) {
    destroyMongoClient();
    return liret::kAborted;
  }

  return liret::kOk;
}

liret MongoClient::ping(bson_error_t *error) {
  mongoc_client_t *client = nullptr;
  liret ret = liret::kOk;
  bson_t ping = BSON_INITIALIZER;
  BSON_APPEND_INT32(&ping, "ping", 1);

  do {
    client = mongoc_client_pool_pop(this->m_cPool);
    if (client == nullptr) {
      ret = liret::kUninitialized;
      break;
    }
    // mongoc_client_set_sockettimeoutms(client, 200);
    bool r = mongoc_client_command_simple(client, m_dbName, &ping, NULL, NULL, error);

    if (!r) {
      ret = liret::kAborted;
      break;
    }
    break;
  } while (0);
  if (client != nullptr) {
    mongoc_client_pool_push(m_cPool, client);
  }
  return ret;
}

liret MongoClient::getImageById(const char *id, size_t size, unsigned char **filedata, size_t *filesize) {
  if (size != 24) {
    return liret::kInvalidInput;
  }

  bson_oid_t oid;
  bson_oid_init_from_string(&oid, id);

  mongoc_client_t *client = nullptr;
  mongoc_database_t *database = nullptr;
  mongoc_gridfs_bucket_t *bucket = nullptr;
  mongoc_stream_t *stream = nullptr;

  int bufSize = 0;
  unsigned char *outBuf = nullptr;

  char buf[MONGO_TRANSFER_CHUNK];
  mongoc_iovec_t iov;
  iov.iov_base = buf;
  iov.iov_len = sizeof buf;

  liret ret = liret::kOk;
  do {
    if (this->m_cPool == nullptr) {
      ret = liret::kUninitialized;
      break;
    }
    client = mongoc_client_pool_pop(this->m_cPool);
    if (client == nullptr) {
      ret = liret::kUninitialized;
      break;
    }

    database = mongoc_client_get_database(client, m_dbName);
    if (database == nullptr) {
      ret = liret::kAborted;
      break;
    }

    bucket = mongoc_gridfs_bucket_new(database, nullptr, nullptr, nullptr);
    if (bucket == nullptr) {
      ret = liret::kAborted;
      break;
    }

    bson_value_t oid_value;
    oid_value.value_type = BSON_TYPE_OID;
    memcpy(&oid_value.value.v_oid, &oid, sizeof(bson_oid_t));

    bson_error_t error = {0};
    stream = mongoc_gridfs_bucket_open_download_stream(bucket, &oid_value, &error);
    if (error.code == MONGOC_ERROR_GRIDFS_BUCKET_FILE_NOT_FOUND) {
      ret = liret::kNotFound;
      break;
    }
    if (stream == nullptr) {
      ret = liret::kUnknown;
      break;
    }

    while (true) {
      ssize_t r = mongoc_stream_readv(stream, &iov, 1, -1, 0);
      if (r == 0) {
        break;
      }
      if (outBuf == nullptr) {
        outBuf = (unsigned char *)malloc(r);
      } else {
        outBuf = (unsigned char *)realloc(outBuf, bufSize + r);
      }
      memcpy(outBuf + bufSize, iov.iov_base, r);
      bufSize += r;
    }
    *filedata = outBuf;
    *filesize = bufSize;

  } while (0);

  if (client != nullptr) {
    mongoc_client_pool_push(m_cPool, client);
  }
  if (database != nullptr) {
    mongoc_database_destroy(database);
  }
  if (bucket != nullptr) {
    mongoc_gridfs_bucket_destroy(bucket);
  }
  if (stream != nullptr) {
    mongoc_stream_destroy(stream);
  }

  return ret;
}

liret MongoClient::updateImageById(const char *id, size_t size, unsigned char *filedata, size_t filesize) {
  if (size != 24) {
    return liret::kInvalidInput;
  }
  bson_oid_t oid;
  bson_oid_init_from_string(&oid, id);

  mongoc_gridfs_t *gridfs = nullptr;
  mongoc_gridfs_file_t *file = nullptr;
  mongoc_client_t *client = nullptr;
  mongoc_database_t *database = nullptr;
  mongoc_gridfs_bucket_t *bucket = nullptr;
  mongoc_stream_t *stream = nullptr;

  liret ret = liret::kOk;
  do {
    if (this->m_cPool == nullptr) {
      ret = liret::kUninitialized;
      break;
    }
    client = mongoc_client_pool_pop(this->m_cPool);
    if (client == nullptr) {
      ret = liret::kUninitialized;
      break;
    }

    database = mongoc_client_get_database(client, m_dbName);
    if (database == nullptr) {
      ret = liret::kAborted;
      break;
    }

    bucket = mongoc_gridfs_bucket_new(database, nullptr, nullptr, nullptr);
    if (bucket == nullptr) {
      ret = liret::kAborted;
      break;
    }
    gridfs = mongoc_client_get_gridfs(client, this->m_dbName, "fs", nullptr);
    if (gridfs == nullptr) {
      ret = liret::kAborted;
      break;
    }

    bson_t query = BSON_INITIALIZER;
    BSON_APPEND_OID(&query, "_id", &oid);
    bson_error_t error = {0};
    file = mongoc_gridfs_find_one(gridfs, &query, &error);
    if (error.code == MONGOC_ERROR_GRIDFS_BUCKET_FILE_NOT_FOUND) {
      ret = liret::kNotFound;
      break;
    }
    if (file == nullptr) {
      ret = liret::kUnknown;
      break;
    }
    // fname cleanup automatic
    const char *fname = mongoc_gridfs_file_get_filename(file);
    const bson_t *metadata = mongoc_gridfs_file_get_metadata(file);
    bson_value_t oid_value;
    oid_value.value_type = BSON_TYPE_OID;
    memcpy(&oid_value.value.v_oid, &oid, sizeof(bson_oid_t));

    if (mongoc_gridfs_file_remove(file, &error) == false) {
      ret = liret::kUnknown;
      break;
    }

    mongoc_stream_gridfs_new(file);
    bson_t options = BSON_INITIALIZER;
    BSON_APPEND_DOCUMENT(&options, "metadata", metadata);
    stream = mongoc_gridfs_bucket_open_upload_stream_with_id(bucket, &oid_value, fname, &options, &error);
    if (stream == nullptr) {
      ret = liret::kUnknown;
      break;
    }
    const int chunkSize = MONGO_TRANSFER_CHUNK;
    const int totalChunks = filesize / chunkSize;
    const int restChunk = filesize - (chunkSize * totalChunks);
    ssize_t total = 0;
    // To prevent unnessesary branching after loop just load rest of the image
    for (int i = 0; i < totalChunks; i++) {
      ssize_t r = mongoc_stream_write(stream, filedata + (chunkSize * i), chunkSize, 0);
      total += r;
    }
    if (restChunk > 0) {
      ssize_t r = mongoc_stream_write(stream, (filedata) + (filesize - restChunk), restChunk, 0);
      total += r;
    }
  } while (0);

  if (file != nullptr) {
    mongoc_gridfs_file_destroy(file);
  }
  if (gridfs != nullptr) {
    mongoc_gridfs_destroy(gridfs);
  }
  if (stream != nullptr) {
    mongoc_stream_destroy(stream);
  }
  if (bucket != nullptr) {
    mongoc_gridfs_bucket_destroy(bucket);
  }
  if (database != nullptr) {
    mongoc_database_destroy(database);
  }
  if (client != nullptr) {
    mongoc_client_pool_push(m_cPool, client);
  }

  return ret;
}

int MongoClient::mongoTest() {
  mongoc_client_t *client;
  mongoc_collection_t *collection;
  mongoc_cursor_t *cursor;
  bson_error_t error;
  const bson_t *doc;
  const char *collection_name = "userBase";
  bson_t query;
  char *str;
  const char *uri_string = "mongodb://192.168.1.101:8005/?appname=client-example";
  mongoc_uri_t *uri;

  uri = mongoc_uri_new_with_error(uri_string, &error);
  if (!uri) {
    fprintf(stderr,
            "failed to parse URI: %s\n"
            "error message:       %s\n",
            uri_string, error.message);
    return EXIT_FAILURE;
  }

  client = mongoc_client_new_from_uri(uri);
  if (!client) {
    return EXIT_FAILURE;
  }

  mongoc_client_set_error_api(client, 2);

  bson_init(&query);
  collection = mongoc_client_get_collection(client, "LimbDB", collection_name);
  cursor = mongoc_collection_find_with_opts(collection, &query, NULL, /* additional options */
                                            NULL);                    /* read prefs, NULL for default */

  while (mongoc_cursor_next(cursor, &doc)) {
    str = bson_as_canonical_extended_json(doc, NULL);
    fprintf(stdout, "%s\n---\n", str);
    bson_free(str);
  }

  if (mongoc_cursor_error(cursor, &error)) {
    fprintf(stderr, "Cursor Failure: %s\n", error.message);
    return EXIT_FAILURE;
  }

  bson_destroy(&query);
  mongoc_cursor_destroy(cursor);
  mongoc_collection_destroy(collection);
  mongoc_uri_destroy(uri);
  mongoc_client_destroy(client);
  mongoc_cleanup();

  return EXIT_SUCCESS;
}
} // namespace limb