#pragma once

#include <memory>
#include <string>

#include "Constant.h"
#include "pybind11/pybind11.h"

namespace dolphindb {

namespace py = pybind11;

class PythonObjectHolder {
public:
    PythonObjectHolder() = default;

    explicit PythonObjectHolder(const py::handle& obj)
        : PythonObjectHolder(py::reinterpret_borrow<py::object>(obj)) {}

    explicit PythonObjectHolder(py::object obj)
        : object_(new py::object(std::move(obj)), &PythonObjectHolder::destroy) {}

    explicit operator bool() const {
        return static_cast<bool>(object_);
    }

    bool empty() const {
        return !object_;
    }

    const py::object& get() const {
        return *object_;
    }

    py::object& get() {
        return *object_;
    }

private:
    static void destroy(py::object* value) {
        if (value == nullptr || !Py_IsInitialized()) {
            return;
        }
        py::gil_scoped_acquire acquire;
        delete value;
    }

private:
    std::shared_ptr<py::object> object_;
};

enum REQUEST_FORMAT {
    REQUEST_FORMAT_AUTO = -1,
    REQUEST_FORMAT_DDB = 0,
    REQUEST_FORMAT_ARROW = 2,
    REQUEST_FORMAT_ARROW_RETURN_LIMIT = 6,
};

struct RequestArgument {
    enum Kind {
        CONSTANT,
        RAW_BYTES,
        ARROW_TABLE,
    };

    RequestArgument() = default;
    explicit RequestArgument(const ConstantSP& obj) : kind(CONSTANT), constant(obj) {}
    explicit RequestArgument(std::string payload)
        : kind(RAW_BYTES), raw(std::move(payload)) {}
    static RequestArgument fromArrowTable(PythonObjectHolder object) {
        RequestArgument arg;
        arg.kind = ARROW_TABLE;
        arg.pythonObject = std::move(object);
        return arg;
    }

    bool isRaw() const {
        return kind == RAW_BYTES;
    }

    bool isConstant() const {
        return kind == CONSTANT;
    }

    bool isArrowTable() const {
        return kind == ARROW_TABLE;
    }

    Kind kind = CONSTANT;
    ConstantSP constant;
    std::string raw;
    PythonObjectHolder pythonObject;
};

}  // namespace dolphindb
