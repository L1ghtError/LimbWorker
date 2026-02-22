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
- [ ] Add Protobuffs payload type for Rabbitmq
- [ ] Fix CRUSH when usesr pass image with wrong format

## 🔴 Implementation Tasks (1.5) 
- [x] Implement app configuration file
- [x] Remove config usage from all modules and move it to the `main`
- [ ] Optimize image transcoding with `libturbojpeg`
- [ ] Add pre-commit hooks

📘 Dictionary
Processor — A module responsible for enhancing or processing provided content, such as images, audio, or other media types. Processors handle specific transformation or optimization tasks, often dynamically loaded depending on the content type.
