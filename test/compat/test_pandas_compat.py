"""Offline regression tests: python -I -m unittest discover -s test/compat -v."""
import unittest
from contextlib import nullcontext

import numpy as np
import pandas as pd
import pyarrow as pa

from dolphindb import io


class PandasCompatibilityTest(unittest.TestCase):
    def frame(self, values, unit="ns"):
        array = pa.array(values, type=pa.timestamp(unit))
        return pd.DataFrame({"ts": pd.Series(array, dtype=pd.ArrowDtype(array.type))})

    def test_units_and_input_unchanged(self):
        for unit, scale in [("s", 10**9), ("ms", 10**6), ("ns", 1)]:
            for values in ([1234567, -1234567, 0, None], [None, None], []):
                with self.subTest(unit=unit, values=values):
                    frame = self.frame(values, unit)
                    before = frame.copy(deep=True)
                    result = io.loads(io.dumps(frame))
                    expected = np.array([
                        np.iinfo(np.int64).min if value is None else value * scale
                        for value in values
                    ], dtype="int64").view("datetime64[ns]")
                    np.testing.assert_array_equal(result.ts.to_numpy(), expected)
                    pd.testing.assert_frame_equal(frame, before)

    def test_string_backends(self):
        for dtype in [None, "object", "string[python]", "string[pyarrow]"]:
            with self.subTest(dtype=dtype):
                frame = pd.DataFrame({"s": pd.Series(["中文", "abc", None, ""], dtype=dtype)})
                before = frame.copy(deep=True)
                result = io.loads(io.dumps(frame))
                self.assertEqual(result.s.tolist(), ["中文", "abc", "", ""])
                pd.testing.assert_frame_equal(frame, before)

    def test_nullable_numbers_and_copy_on_write(self):
        for dtype in ["float32", "float64", "Float32", "Float64", "Int64"]:
            cow = pd.option_context("mode.copy_on_write", True) if pd.__version__.startswith("2.") else nullcontext()
            with self.subTest(dtype=dtype), cow:
                frame = pd.DataFrame({"x": pd.Series([1, None, -3], dtype=dtype)})
                before = frame.copy(deep=True)
                result = io.loads(io.dumps(frame))
                np.testing.assert_allclose(
                    result.x.to_numpy(dtype=float), [1, np.nan, -3], equal_nan=True
                )
                pd.testing.assert_frame_equal(frame, before)

    def test_default_datetime_resolution(self):
        frame = pd.DataFrame({"ts": pd.to_datetime(["2026-01-02 03:04:05.123456", None])})
        before = frame.copy(deep=True)
        result = io.loads(io.dumps(frame))
        np.testing.assert_array_equal(
            result.ts.to_numpy(), frame.ts.to_numpy().astype("datetime64[ns]")
        )
        pd.testing.assert_frame_equal(frame, before)


if __name__ == "__main__":
    unittest.main()
