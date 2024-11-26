## Limb Worker

### Image procesing service written on C++

Works with [Backend](https://github.com/L1ghtError/LimbService).

<div style="border-left: 4px solid #2196F3; padding: 0.5em; background: #f1f9ff;">
  <strong>NOTE:</strong> Develop in progress.
</div>

<div style="border-left: 4px solid #FF9800; padding: 0.5em; background: #fff3e0; margin-top: 1em;">
  <strong>WARNING:</strong>
  <ul>
    <li>Application is <strong>NOT</strong> even Alpha.</li>
    <li>It can <strong>CRASH</strong> if the user passes an image with the wrong format.</li>
    <li>All server configuration is <strong>Hardcoded</strong>.</li>
    <li>Only the <strong>precompiled</strong> version of <code>mongo-c-driver</code> is suitable.</li>
  </ul>
</div>

<div style="border-left: 4px solid #4CAF50; padding: 0.5em; background: #f0fdf4; margin-top: 1em;">
  <strong>TIP:</strong> In its current state, it should be used only as an example for your own improvements.
</div>

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
