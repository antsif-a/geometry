#!/usr/bin/env python3
import xml.etree.ElementTree as ET

tree = ET.parse('gl.xml')
root = tree.getroot()

TARGET_VERSION = "4.6"

# 1. Parse all available definitions into lookups
all_enums = {}
for e in root.findall('.//enums/enum'):
    all_enums[e.get('name')] = {
        'value': e.get('value'),
        'type': e.get('type')  # Can be None, 'u', or 'ull'
    }
all_commands = {}
for c in root.find('.//commands').findall('.//command'):
    proto = c.find('.//proto')
    name = proto.find('.//name').text
    ret = "".join(proto.itertext())[:-"".join(proto.find('.//name').itertext()).__len__()].strip()
    params = ["".join(p.itertext()).strip() for p in c.findall('.//param')]
    all_commands[name] = (ret, params)

# 2. Identify required names for the target version
required_enums = set()
required_commands = set()

for feature in root.findall('.//feature'):
    if feature.get('api') == 'gl' and float(feature.get('number')) <= float(TARGET_VERSION):
        for req in feature.findall('require'):
            for e in req.findall('enum'):
                required_enums.add(e.get('name'))
            for c in req.findall('command'):
                required_commands.add(c.get('name'))
        
        # Handle removals (Core Profile)
        for rem in feature.findall('remove'):
            for e in rem.findall('enum'):
                required_enums.discard(e.get('name'))
            for c in rem.findall('command'):
                required_commands.discard(c.get('name'))

# 3. Write the Module
with open('glcore.cppm', 'w') as file:
    write = lambda x: file.write(x + '\n')

    write('module;')
    write('#include <stddef.h>\n#include <stdint.h>\n#include "khrplatform.h"\n')
    write('export module gl.core;')

    # Types (Most are usually required for basic functionality)
    write('export extern "C" {')
    for t in root.find('.//types').findall('.//type'):
        if t.get('name') == 'khrplatform': continue
        t_str = "".join(t.itertext()).strip()
        if not t_str or t_str.startswith('#include'): continue
        write(f'    {t_str}{"" if t_str.endswith(";") or t_str.startswith("#") else ";"}')
    write('}\n')

    # Filtered Enums
    for name in sorted(required_enums):
        if name in all_enums:
            val = all_enums[name]['value']
            enum_type = all_enums[name]['type']

            # Default to unsigned int (standard GLenum), use uint64_t for 'ull'
            cpp_type = "unsigned long long" if enum_type == "ull" else "unsigned int"

            write(f'export constexpr {cpp_type} {name} = {val};')

    # Filtered Commands
    write('\nexport extern "C" {')
    for name in sorted(required_commands):
        if name in all_commands:
            ret, params = all_commands[name]
            write(f'    {ret} {name}({", ".join(params) if params else "void"});')
    write('}')
