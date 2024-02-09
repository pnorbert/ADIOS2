"""License:
  Distributed under the OSI-approved Apache License, Version 2.0.  See
  accompanying file Copyright.txt for details.
"""

from __future__ import annotations

import os
# import warnings

from collections.abc import Iterable
from typing import TYPE_CHECKING, Any, ItemsView

import numpy as np
from adios2 import FileReader

# from xarray.core.dataset import Dataset
from xarray import Dataset, Variable
from xarray import coding, conventions
from xarray.backends.common import (
    BACKEND_ENTRYPOINTS,
    AbstractWritableDataStore,
    BackendArray,
    BackendEntrypoint,
    _encode_variable_name,
    _normalize_path,
)

# from xarray.backends.store import StoreBackendEntrypoint
from xarray.core import indexing

# from xarray.core.indexing import LazilyIndexedArray

# from xarray.core.parallelcompat import guess_chunkmanager
from xarray.core.pycompat import integer_types
from xarray.core.utils import (
    FrozenDict,
    HiddenKeyDict,
    close_on_error,
)

if TYPE_CHECKING:
    from io import BufferedIOBase
    from xarray.backends.common import AbstractDataStore

# need some special secret attributes to tell us the dimensions
DIMENSION_KEY = "time_dimension"

adios_to_numpy_type = {
    "char": np.char,
    "int8_t": np.int8,
    "int16_t": np.int16,
    "int32_t": np.int32,
    "int64_t": np.int64,
    "uint8_t": np.uint8,
    "uint16_t": np.uint16,
    "uint32_t": np.uint32,
    "uint64_t": np.uint64,
    "float": np.float,
    "double": np.double,
    "long double": np.longdouble,
    "float complex": np.complex64,
    "double complex": np.complex128,
    "string": np.char,
}


class ADIOSBackendArray(BackendArray):
    '''ADIOS2 backend for lazily indexed arrays '''
    def __init__(self, shape, dtype, lock, adiosfile: FileReader, varname: str):
        self.shape = shape
        self.dtype = dtype
        self.lock = lock
        self.fh = adiosfile
        self.varname = varname
        self.adiosvar = self.fh.inquire_variable(varname)
        self.steps = self.adiosvar.steps()
        # print(f"ADIOSBackendArray.__init__: {dtype} {varname} {shape}")

    def __getitem__(self, key: indexing.ExplicitIndexer) -> np.typing.ArrayLike:
        # print(f"ADIOSBackendArray.__getitem__: {self.varname} key = {key}")

        return indexing.explicit_indexing_adapter(
            key,
            self.shape,
            indexing.IndexingSupport.BASIC,
            self._raw_indexing_method,
        )

    def _raw_indexing_method(self, key: tuple) -> np.typing.ArrayLike:
        # print(f"ADIOSBackendArray._raw_indexing_method:
        #    {self.varname} key = {key} steps = {self.steps}")
        # thread safe method that access to data on disk
        # with self.lock:
        self.adiosvar.set_step_selection([0, self.steps])
        data = self.fh.read(self.adiosvar)
        if self.steps > 1:
            # ADIOS does not have time dimension. Read returns n-dim array
            # with the steps included in the first dimension
            dim0 = int(data.shape[0] / self.steps)
            if data.shape[0] % self.steps != 0:
                print(
                    f"ERROR in ADIOSBackendArray: first dimension problem "
                    f"with handling steps. Variable name={self.varname} "
                    f"shape={data.shape}, steps={self.steps}"
                )
            data = data.reshape((self.steps, dim0) + data.shape[1:])
        # print(f"    data shape {data.shape}")
        return data


def attrs_of_var(varname: str, items: ItemsView, separator: str = "/"):
    '''Return attributes whose name starts with a variable's name'''
    return [(key, value) for key, value in items if key.startswith(varname + separator)]


# pylint: disable=R0902   # Too many instance attributes
# pylint: disable=R0912   # Too many branches
# pylint: disable=E1121   # too-many-function-args
class AdiosBackendEntrypoint(BackendEntrypoint):
    """
    Backend for ".bp" folders based on the adios2 package.

    For more information about the underlying library, visit:
    https://adios2.readthedocs.io/en/stable

    See Also
    --------
    backends.AdiosStore
    """

    description = "Open ADIOS2 files/folders (.bp) using adios2 in Xarray"
    url = "https://docs.xarray.dev/en/stable/generated/xarray.backends.ZarrBackendEntrypoint.html"

    def __init__(self):
        self._fh = None

    def close():
        '''Close the ADIOS file'''
        print("AdiosBackendEntrypoint.close() called")
        # Note that this is a strange method without 'self', so we cannot close the file because
        # we don't have any handle to it
        #        if self._fh is not None:
        #            self._fh.close()

    def guess_can_open(
        self,
        filename_or_obj: str | os.PathLike[Any] | BufferedIOBase | AbstractDataStore,
    ) -> bool:
        if isinstance(filename_or_obj, (str, os.PathLike)):
            _, ext = os.path.splitext(filename_or_obj)
            return ext in {".bp"}

        return False

    def open_dataset(  # type: ignore[override]  # allow LSP violation, not supporting **kwargs
        self,
        filename_or_obj: str | os.PathLike[Any] | BufferedIOBase | AbstractDataStore,
        *,
#        mask_and_scale=True,
#        decode_times=True,
#        concat_characters=True,
#        decode_coords=True,
        drop_variables: str | Iterable[str] | None = None,
#        use_cftime=None,
#        decode_timedelta=None,
#        group=None,
#        mode="r",
#        synchronizer=None,
#        consolidated=None,
#        chunk_store=None,
#        storage_options=None,
#        stacklevel=3,
#        adios_version=None,
    ) -> Dataset:
        filename_or_obj = _normalize_path(filename_or_obj)
        print(f"AdiosBackendEntrypoint: path = {filename_or_obj} type = {type(filename_or_obj)}")

        if isinstance(filename_or_obj, os.PathLike):
            print(f"    os.PathLike: {os.fspath(filename_or_obj)}")

        if isinstance(filename_or_obj, str):
            print(f"    str: {os.path.abspath(os.path.expanduser(filename_or_obj))}")

        #        if isinstance(filename_or_obj, BufferedIOBase):
        #            raise ValueError("ADIOS2 does not support BufferedIOBase input")
        #
        #        if isinstance(filename_or_obj, AbstractDataStore):
        #            raise ValueError("ADIOS2 does not support AbstractDataStore input")

        self._fh = FileReader(filename_or_obj)
        vars = self._fh.available_variables()
        attrs = self._fh.available_attributes()
        attr_items = attrs.items()
        print(f"AdiosBackendEntrypoint: {len(vars)} variables, {len(attrs)} attributes")
        xvars = {}

        for varname, varinfo in vars.items():
            if drop_variables is not None and varname in drop_variables:
                continue
            shape_str = varinfo["Shape"].split(", ")
            if shape_str[0]:
                shape_list = list(map(int, shape_str))
            else:
                shape_list = []
                shape_str = []
            steps = int(varinfo["AvailableStepsCount"])
            # print(f"{varinfo['Type']} {varname}\t {shape_list}")
            varattrs = attrs_of_var(varname, attr_items, "/")
            dims = None
            vlen = len(varname) + 1  # include /
            xattrs = {}
            for aname, ainfo in varattrs:
                # print(f"\t{ainfo['Type']} {aname}\t = {ainfo['Value']}")
                attr_value = self._fh.read_attribute(aname)
                if aname == varname + "/__xarray_dimensions__":
                    dims = attr_value
                    # print(f"\t\tDIMENSIONS = {dims}")
                else:
                    xattrs[aname[vlen:]] = attr_value
                attrs.pop(aname)
            # print(f"\txattrs = {xattrs}")

            # Create the xarray variable
            if dims is None:
                dims = shape_str
            if shape_list != []:
                # for i in range(len(shape_str)):
                #    shape_str[i] = "d" + shape_str[i]
                if steps > 1:
                    shape_list.insert(0, steps)
                    dims.insert(0, "t")
                    # print(f"\tAdd time to shape {shape_list}  {dims}")
                nptype = adios_to_numpy_type[varinfo["Type"]]
                xdata = indexing.LazilyIndexedArray(
                    ADIOSBackendArray(shape_list, nptype, None, self._fh, varname)
                )
                # print(f"\tDefine VARIABLE {varname} with dims {dims}")
                xvar = Variable(dims, xdata, attrs=xattrs, encoding=None)
            else:
                if steps > 1:
                    avar = self._fh.inquire_variable(varname)
                    avar.set_step_selection([0, avar.steps()])
                    data = self._fh.read(avar)
                    # print(f"\tCreate timed scalar variable {varname}")
                    xvar = Variable("t", data, attrs=xattrs, encoding=None)
                else:
                    data = self._fh.read(varname)
                    if varinfo["Type"] == "string":
                        # print(f"\tCreate string scalar variable {varname}")
                        xvar = Variable([], data, attrs=xattrs, encoding=None)
                    else:
                        # print(f"\tCreate scalar variable {varname}")
                        xvar = Variable([], data[0], attrs=xattrs, encoding=None)
            xvars[varname] = xvar
            # print(f"--- {xvar}")

        for attname, attinfo in attrs.items():
            print(f"{attinfo['Type']} {attname}\t = {attinfo['Value']}")

        ds = Dataset(xvars, None, None)
        ds.set_close(AdiosBackendEntrypoint.close)
        return ds
