#include <emscripten/bind.h>

using namespace emscripten;

float lerp(float a, float b, float t) {
    return (1 - t) * a + t * b;
}

std::string getGreeting() {
    return "Hello from C++!";
}

std::string echoString(const std::string& input) {
    return "Echo: " + input;
}

EMSCRIPTEN_BINDINGS(my_module) {
    function("lerp", &lerp);
    function("getGreeting", &getGreeting);
    function("echoString", &echoString);
}
