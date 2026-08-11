#ifndef __WRAPPERS_DOLPHINDB_H
#define __WRAPPERS_DOLPHINDB_H

#include "DolphinDB.h"
#include "Exceptions.h"
#include "Types.h"
#include "pybind11/pybind11.h"

namespace py = pybind11;

namespace dolphindb{

class EXPORT_DECL InputStreamWrapper{
public:
    InputStreamWrapper(){};
    void setInputStream(DataInputStreamSP inputStream){
        _inputStream = inputStream;
    }
    py::bytes read(size_t size) {
        std::unique_ptr<char[]> bufsp(new char[size+1]);
        char *buf = bufsp.get();
        size_t actual_len = -1;
        IO_ERR ret = _inputStream->readBytes(buf, size, actual_len);
        if (ret != IO_ERR::OK) {
            throw IOException("Failed to parse the incoming object with IO error type " + std::to_string(ret), ret);
        }
        return py::bytes(buf, actual_len);
    }
    bool closed(){
        return !(_inputStream->getSocket()->isValid());
    }
private:
    DataInputStreamSP _inputStream;
};

class EXPORT_DECL OutputStreamWrapper{
public:
    OutputStreamWrapper(){};
    void setOutputStream(DataOutputStreamSP outputStream){
        _outputStream = outputStream;
    }
    py::ssize_t write(py::object data) {
        const char *buffer = nullptr;
        py::ssize_t length = 0;

        Py_buffer view{};
        bool hasBufferView = false;
        if (PyObject_CheckBuffer(data.ptr())) {
            if (PyObject_GetBuffer(data.ptr(), &view, PyBUF_CONTIG_RO) == 0) {
                hasBufferView = true;
                buffer = static_cast<const char *>(view.buf);
                length = static_cast<py::ssize_t>(view.len);
            } else {
                PyErr_Clear();
            }
        }

        py::object bytes;
        if (!hasBufferView) {
            if (PyBytes_Check(data.ptr())) {
                bytes = data;
            } else {
                PyObject *converted = PyBytes_FromObject(data.ptr());
                if (converted == nullptr) {
                    throw py::error_already_set();
                }
                bytes = py::reinterpret_steal<py::object>(converted);
            }
            char *rawBuffer = nullptr;
            if (PyBytes_AsStringAndSize(bytes.ptr(), &rawBuffer, &length) < 0) {
                throw py::error_already_set();
            }
            buffer = rawBuffer;
        }

        if (length == 0) {
            if (hasBufferView) {
                PyBuffer_Release(&view);
            }
            return 0;
        }

        size_t actualWritten = 0;
        IO_ERR ret = OK;
        try {
            ret = _outputStream->write(buffer, static_cast<size_t>(length), actualWritten);
        } catch (...) {
            if (hasBufferView) {
                PyBuffer_Release(&view);
            }
            throw;
        }
        if (hasBufferView) {
            PyBuffer_Release(&view);
        }
        if (ret != IO_ERR::OK) {
            throw IOException("Failed to write the outgoing object with IO error type " + std::to_string(ret), ret);
        }
        if (actualWritten != static_cast<size_t>(length)) {
            throw IOException("Failed to write the complete outgoing object to the socket.");
        }
        return static_cast<py::ssize_t>(actualWritten);
    }
    void flush() {
        IO_ERR ret = _outputStream->flush();
        if (ret != IO_ERR::OK) {
            throw IOException("Failed to flush the outgoing object with IO error type " + std::to_string(ret), ret);
        }
    }
    void close(){
        closed_ = true;
    }
    bool closed() const{
        return closed_ || _outputStream.isNull() || !(_outputStream->getSocket()->isValid());
    }
    bool writable() const {
        return true;
    }
private:
    DataOutputStreamSP _outputStream;
    bool closed_ = false;
};

}

#endif // __WRAPPERS_DOLPHINDB_H
