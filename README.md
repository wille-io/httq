# httq

HTTP server library for Qt. Doesn't clog the memory like The Other HTTP Server™.

Based on NodeJS's wonderful http-parser.
Can download a message's body in chunks, instead of reading it all into memory that allows an easy DoS, like The Other HTTP Server™.

## CMake Integration

### Option A: Add as Subdirectory (recommended for local use)

```
add_subdirectory(path/to/httq)

add_executable(my_app main.cpp)
target_link_libraries(my_app
  PRIVATE
    httq
    Qt${Qt_VERSION_MAJOR}::Core
    Qt${Qt_VERSION_MAJOR}::Network
    Qt${Qt_VERSION_MAJOR}::WebSockets
)
target_include_directories(my_app PRIVATE path/to/httq/include)
```

### Option B: Installed Library via find_package

```
find_package(httq REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE httq::httq)
```

> Note: This works after installing httq (exports targets and config files).
> For local projects, option A is currently the simplest approach.

### Installing httq

```
cmake -S . -B build
cmake --build build
sudo cmake --install build
```

## C++ Examples

### Minimal HTTP Handler

```cpp
#include <httq/HandlerServer.h>
#include <httq/AbstractHandler.h>

class HelloHandler : public httq::AbstractHandler
{
public:
  void handle() override
  {
    answer(200, "Hello from httq");
  }
};

int main(int argc, char **argv)
{
  QCoreApplication app(argc, argv);

  httq::HandlerServer server;
  server.addHandler("GET", "/hello", []() { return new HelloHandler(); });
  server.listen(8080);

  return app.exec();
}
```

### JSON Response

```cpp
#include <httq/HandlerServer.h>
#include <httq/AbstractHandler.h>

class JsonHandler : public httq::AbstractHandler
{
public:
  void handle() override
  {
    QJsonObject obj { { "ok", true } };
    answer(200, obj);
  }
};
```

### Body Handling with AbstractBodyHandler

```cpp
#include <httq/AbstractBodyHandler.h>

class EchoHandler : public httq::AbstractBodyHandler
{
public:
  using httq::AbstractBodyHandler::AbstractBodyHandler;

  void bodyHandle() override
  {
    answer(200, body(), "text/plain");
  }
};
```

## Build and Run Tests (QTest)

```
cmake -S . -B build -DHTTQ_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```
