## Limb Worker

### Image procesing service written on C++

Works with [Backend](https://github.com/L1ghtError/LimbService).

> [!NOTE]  
> **Develop in progress**

> [!WARNING]  
> Application is **NOT** event Alpha.
> There is **NO** Multithreading.
> Also it can **CRUSH** if usesr pass image with wrong format.
> All server configuration is **HardCoded**.
> Only **precompiled** version of mongo-c-driver is sutable.

> [!TIP]
> In current state it should be used only as example for your own improvments

### How to build

native:

```bash
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug
$ cmake --build .
$ .\light-backend
```

Docker:

```bash
** in future updates**
```

Docker-compose:

```bash
** in future updates**
```

> **Tech stack:**
>
> - [ncnn](https://github.com/Tencent/ncnn) as web-framework
> - [mongo-c-driver](https://github.com/mongodb/mongo-c-driver) for communication with workers
> - [AMQP-CPP](https://github.com/CopernicaMarketingSoftware/AMQP-CPP) for communication with MongoDb
