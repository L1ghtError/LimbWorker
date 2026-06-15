### ✅ Refactoring list
### Goals
>- Implement Hexagonal/Onion architecture
>- Pass validations such as `Valgrind`, `Cppcheck`
>- Make code more robust

## 🔴 Refactoring Tasks To-Do High Priority (1)
- [x] Implement dynamic image processor loading
- [x] Implemnet Repository instead of explicit usage of mongodb
- [x] Fix mongo-c build on Windows
- [x] Implement service that provides unified acess to processors
- [x] Increase test coverage
- [x] Fix CRUSH when usesr pass image with wrong format
- [ ] Add processor unloading machanism
- [-] Add Protobuffs payload type for Rabbitmq

## 🔴 Implementation Tasks (1.5) 
- [x] Implement app configuration file
- [x] Remove config usage from all modules and move it to the `main`
- [x] Add pre-commit hooks
- [x] Implementa abstraction for image format decoding/encoding
- [x] Optimize jpeg image transcoding with `libturbojpeg`
- [ ] Separate generation logic in CMake by splitting one file into multiple organized files
- [ ] make onnxruntime shared lib visible for rmbg processors without manual
- [ ] Implement valid program instalation
- [ ] Fork AMQP cpp and raise cmake minimum version
- [ ] Update metadata in database if input was jpg and output is png


📘 Dictionary
Processor — A module responsible for enhancing or processing provided content, such as images, audio, or other media types. Processors handle specific transformation or optimization tasks, often dynamically loaded depending on the content type.
