"""
Module extending the Pixmap class with .text submodule and its text drawing functions.

Use as in "import gfxprim.text; pixmap_foo.text.text(...)"
"""

# Import the SWIG wrapper
from . import c_text

# Constants module
from . import C

def _init(module):
  "Extend pixmap with text submodule"

  from ..utils import extend, add_swig_getmethod, add_swig_setmethod
  from ..core import Pixmap as _pixmap

  # New Pixmap submodule
  class TextSubmodule(object):
    def __init__(self, ctx):
      self.ctx = ctx
      self.C = C

  _pixmap._submodules['text'] = TextSubmodule

  # Imports from the SWIG module
  from ..utils import import_members, extend_submodule
  import re
  def strip_GP(s):
    return re.sub('^gp_|^GP_', '', s)

  const_regexes = [
    '^GP_[A-Z0-9_]*$',
    ]
  import_members(c_text, C, include=const_regexes, sub=strip_GP)

  import_members(c_text, module, sub=strip_GP)

  for name in ['text']:
    extend_submodule(TextSubmodule, name, c_text.__getattribute__('gp_' + name))

_init(locals())
del _init
