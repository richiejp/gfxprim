from . import gfxprim_core_c as core_c

Context = core_c.Context

# Extend Context with convenience methods
from . import extend_context
extend_context.extend_context_class(Context)
del extend_context

def import_helper(module):
  from ..utils import import_members

  const_regexes = [
      '^GP_[A-Z0-9_]*$',
      '^GP_PIXEL_x[A-Z0-9_]*$']

  # Constants, consider a separate module
  C = {}
  module['C'] = C
  import_members(core_c, C, include=const_regexes)

  # Functions
  import_members(core_c, module,
    exclude=const_regexes + [
      '^GP_Blit\w+$',
      '^GP_Context\w+$',
      '^GP_PixelSNPrint\w+$',
      '^GP_WritePixels\w+$',
      '^\w+_swigregister$',
      '^cvar$',
      '^_\w+$'])

import_helper(locals())
del import_helper
