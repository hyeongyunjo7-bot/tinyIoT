# AGENTS.md

## Project context

This repository is a fork of tinyIoT, a C-based oneM2M CSE project used for SESLab study/work.

The current research task is related to the oneM2M FCNT resource, also known as <flexContainer>.

The main goal is not to rewrite the whole project. The goal is to understand and carefully improve the existing C implementation.

## Important study background

The user is studying FCNT based on:
- oneM2M TS-0001 Functional Architecture
- oneM2M TS-0004 Service Layer Core Protocol

Important FCNT concepts:
- FCNT means flexContainer.
- FCNT is different from CNT/CIN.
- CNT/CIN usually handles content through container/contentInstance structure.
- FCNT is schema-like and uses containerDefinition and custom attributes.
- FCNT resourceType is 28.
- Important FCNT attributes include:
  - cnd: containerDefinition
  - customAttribute
  - cs: contentSize
  - st: stateTag

## Code analysis rules

When analyzing FCNT code, do not only search function names. Trace the actual Create/Update request flow.

For each relevant function, identify:
1. Which file implements the function.
2. Where the function is called from in the request flow.
3. What input arguments the function receives.
4. What value the function checks first.
5. How cnd, customAttribute, cs, and st are handled.
6. What validation functions are called.
7. What values are calculated or modified.
8. Whether the result is reflected in RTNode.
9. Whether the result is reflected in the database.
10. What response code is returned for invalid requests.

## Coding expectations

- Be conservative.
- Do not make large refactors unless explicitly requested.
- Prefer standard C.
- If non-standard C usage is found, explain it first before changing it.
- Keep changes small and reviewable.
- Preserve existing behavior unless there is a clear bug or standard C issue.
- Before editing code, first summarize the relevant files and functions.

## Build environment

The user is working mainly in Ubuntu/WSL.

Before assuming dependencies are available, inspect the Makefile and project structure.

Known build-related context:
- The project uses make.
- Previous build issues involved dependencies such as libwebsockets, pkg-config, PostgreSQL headers, wolfmqtt, and libcoap.
- Do not assume the project builds cleanly without checking the environment.

## Preferred first step for any Codex task

Before editing files, do this:
1. Locate FCNT-related files.
2. Locate Create and Update request handlers.
3. Trace how FCNT requests flow through the server.
4. Produce a short report.
5. Ask before making broad changes.