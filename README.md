## Limb Worker

### Image procesing service written on C++

Works with [Backend](https://github.com/L1ghtError/LimbService).

> **NOTE:**  
> Develop in progress.

---

> **WARNING:**  
> - Application is **NOT** even Alpha, it lacks robustness.

---

> **TIP:**  
> In its current state, it should be used only as an example for your own improvements.


### How to build

native:

```bash
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug
$ cmake --build .
$ .\limb_app
```

```bash
$ cmake --preset win-x64-debug
$ cd build/win-x64-debug
$ cmake --build .
$ .\limb_app
```

Docker:

```bash
** in future updates**
```

Docker-compose:

```bash
** in future updates**
```

> **Dependencies:**
>
> - [ncnn](https://github.com/Tencent/ncnn) as preferred inference runtime
> - [mongo-c-driver](https://github.com/mongodb/mongo-c-driver) for communication with MongoDb
> - [AMQP-CPP](https://github.com/CopernicaMarketingSoftware/AMQP-CPP) for communication with workers
> - [simdjson](https://github.com/simdjson/simdjson) for communication with workers
