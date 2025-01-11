#!/usr/bin/env python
import inspect
import argparse
from pathlib import Path
import sys

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

def import_module(module_path):
    # if a custom module path is provided, add the directory of the file to the first entry in our sys.path
    if module_path:
        path = Path(module_path)
        if not path.exists():
            print(f'error: provided module_path "{module_path}" does not exist')
        sys.path.insert(0, str(path.parent))
    import maat
    return maat

def generate_pyi(module , fail_on_sig_err: bool) -> str:
    s = 'from enum import IntEnum'

    for name, obj in inspect.getmembers(module):
        print(name)
        if inspect.isclass(obj):
            is_enum = hasattr(obj, '_enum_docs')
            if is_enum:
                classdef = f'{name}(IntEnum)'
            else:
                classdef = f'{name}'
            s += '\n'
            s += f'class {classdef}:\n'
            s += f"    '''{obj.__doc__}'''\n\n"

            for member_name, member in inspect.getmembers(obj):
                if not member_name.startswith('__') and member_name != '_enum_docs':
                    print(member_name)
                    if is_enum:
                        s += f'    {member_name} = {int(member)}\n'
                        if member.__doc__:
                            s += f"    '''{obj._enum_docs[member_name]}'''\n"
                    else:
                        try:
                            sig = get_signature(member)
                        except SignatureError as err:
                            if fail_on_sig_err:
                                raise
                            else:
                                print(f"warning: {err}")
                                sig = f'{member_name}(self, *args, **kwargs)'

                        s += f'    def {sig}:\n'
                        s += f"        '''{member.__doc__}'''\n\n"
                        s += "        ...\n\n"
    return s

def main():
    parser = argparse.ArgumentParser(description="Generate pyi stubs by importing and inspecting the maat module")

    parser.add_argument("output_path", type=str, help="The path to the output file")
    parser.add_argument("--module-path", type=str, help="Path to the module to import for inspection", default="")
    parser.add_argument("--fail-on-missing-sig", action="store_true",
                        help="Whether the script should exit with a non-zero status if it encounters an error")

    args = parser.parse_args()

    output_path = Path(args.output_path)
    print(f'Generating pyi file at {output_path}')
    module = import_module(args.module_path)
    pyi_str = generate_pyi(module, args.fail_on_missing_sig)

    with open(output_path, "w") as f:
        f.write(pyi_str)

if __name__ == '__main__':
    main()
