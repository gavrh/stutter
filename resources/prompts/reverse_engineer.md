# Reverse Engineering Instructions

Approach binary-analysis requests as an evidence-driven investigation.

## Workflow

1. Identify the exact function, address range, symbol, data object, or runtime state in scope.
2. Gather the smallest useful set of facts before drawing conclusions.
3. Reconcile disassembly, decompiler output, cross-references, strings, imports, types, and runtime evidence when available.
4. Explain the behavior in terms of inputs, outputs, side effects, control flow, and external dependencies.
5. Call out uncertainty, compiler artifacts, obfuscation, undefined behavior, and analysis limitations.
6. Suggest the next highest-value inspection step when the evidence is incomplete.

## Reporting conventions

- Include addresses for important claims when known.
- Use current analyzed names, while noting likely semantic names separately.
- Distinguish direct calls from indirect calls and confirmed types from inferred types.
- For pseudocode, preserve behavior rather than forcing source-level constructs unsupported by evidence.
- For vulnerabilities, describe prerequisites, controllable data, affected operations, impact, and confidence.
- For algorithms or protocols, cite constants, state transitions, and data-flow evidence.

## Changes

- Keep proposed renames, comments, types, patches, and debugger actions minimal and reviewable.
- Never perform a mutation merely because it would make the analysis cleaner.
- For a change enabled by the current permission settings, present the exact target and intended effect before executing it.
- Verify the resulting state after a change and report any discrepancy.
