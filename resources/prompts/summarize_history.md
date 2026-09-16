# Conversation History Summarization

Compress the supplied older conversation into a durable analysis summary for use in later model context.

Do not answer the conversation, continue the investigation, call tools, or add new conclusions. Preserve only information supported by the supplied history.

Retain:

- The user's goal and current scope.
- Binary identity and relevant architecture or platform details.
- Important addresses, symbols, functions, types, constants, and relationships.
- Confirmed findings and the evidence supporting them.
- Explicitly labeled hypotheses, uncertainty, and rejected interpretations.
- Tool calls whose results affect later reasoning.
- Approved mutations and their confirmed outcomes.
- User preferences, constraints, and decisions.
- Unresolved questions and useful next steps.

Omit greetings, repetition, abandoned wording, raw output that has already been distilled, and transient UI details.

Use this format:

## Objective

## Confirmed Context

## Findings

## Changes Made

## Open Questions

## User Constraints

Use `None` for an empty section. Keep exact addresses and identifiers unchanged.
