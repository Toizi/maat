import maat
import inspect

class SignatureError(Exception):
    pass

def get_signature(func) -> str:
    print(func.__name__)
    func_name = func.__name__
    if func.__doc__ is None:
        raise RuntimeError(f"Could not find __doc__ on {func_name}, fix docs")

    sig = func.__doc__.splitlines()[0].strip()
    err = None
    if not sig.startswith(func_name):
        err = f'signature does not start with function name: {sig}'
    elif func_name not in sig:
        err = f'function name {func_name} not found in {sig}'
    elif sig.index('(') >= sig.index(')'):
        if '(' not in sig:
            err = f'signature does not contain parentheses: {sig}'
        else:
            err = f'closing parenthesis comes before open: {sig}'

    if err:
        raise SignatureError(f"Could not find signature in first line of {func_name}.__doc__, fix docs.\n  Err: {err}")
    return sig

FAIL_BUILD_ON_MISSING_SIG = False

with open('maat.pyi', 'w') as f:
    f.write('from enum import IntEnum')

    for name, obj in inspect.getmembers(maat):
        print(name)
        if inspect.isclass(obj):
            is_enum = hasattr(obj, '_enum_docs')
            if is_enum:
                classdef = f'{name}(IntEnum)'
            else:
                classdef = f'{name}'
            f.write('\n')
            f.write(f'class {classdef}:\n')
            f.write(f"    '''{obj.__doc__}'''\n\n")

            for member_name, member in inspect.getmembers(obj):
                print(member_name)
                if not member_name.startswith('__') and member_name != '_enum_docs':
                    if is_enum:
                        f.write(f'    {member_name} = {int(member)}\n')
                        if member.__doc__:
                            f.write(f"    '''{obj._enum_docs[member_name]}'''\n")
                    else:
                        try:
                            sig = get_signature(member)
                        except SignatureError as err:
                            if FAIL_BUILD_ON_MISSING_SIG:
                                raise
                            else:
                                print(f"warning: {err}")
                                sig = f'{member_name}(self, *args, **kwargs)'

                        f.write(f'    def {sig}:\n')
                        f.write(f"        '''{member.__doc__}'''\n\n")
            f.flush()
