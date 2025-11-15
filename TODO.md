### ✅ Refactoring list
### Goals
>- Implement Hexagonal/Onion architecture
>- Pass validations such as `Valgrind`, `Cppcheck`
>- Make code more robust

## 🔴 Refactoring Tasks To-Do High Priority (1)
- [x] Implement dynamic image processor loading
- [x] Implemnet Repository instead of explicit usage of mongodb
- [ ] Fix mongo-c build on Windows
- [ ] Implement service that provides unified acess to processors
- [ ] Increase test coverage
- [ ] Change Rabbitmq payload type from raw binary to Protobuffs
- [ ] Fix CRUSH when usesr pass image with wrong format

## 🔴 Implementation Tasks (1.5) 
- [ ] Implement app configuration file
- [ ] Remove config usage from all modules and move it to the `main`
- [ ] Optimize image transcoding with `libturbojpeg`

📘 Dictionary
Processor — A module responsible for enhancing or processing provided content, such as images, audio, or other media types. Processors handle specific transformation or optimization tasks, often dynamically loaded depending on the content type.
