# SESLab tinyIoT / oneM2M FCNT Handoff

## Current situation

This is a SESLab study/work project based on tinyIoT.

The user forked the tinyIoT repository and is studying/modifying it in Ubuntu/WSL.

The assigned work is related to modifying non-standard C code into more standard C where appropriate, while also studying the FCNT resource implementation.

## Repository

Main related repositories:
- ankraft/ACME-oneM2M-CSE was mentioned as a reference project.
- seslabSJU/tinyIoT was used as the main project.
- The user is working with a fork of tinyIoT.

## Current study focus

The current focus is oneM2M FCNT, also called flexContainer.

The user was told:
> "FCNT 리소스 봤어? TS-0001, TS-0004에서."

So the user is studying FCNT based on:
- TS-0001 Functional Architecture
- TS-0004 Service Layer Core Protocol

## FCNT summary

FCNT stands for flexContainer.

Important points:
- FCNT is a oneM2M resource.
- It is different from CNT and CIN.
- CNT is a container.
- CIN is a contentInstance stored under CNT.
- FCNT is more flexible and can include custom attributes.
- FCNT uses cnd, meaning containerDefinition.
- FCNT resourceType is 28.
- FCNT has attributes such as:
  - cnd: containerDefinition
  - customAttribute
  - cs: contentSize
  - st: stateTag

## What needs to be checked in tinyIoT C code

When checking FCNT implementation, inspect:

1. FCNT-related source files
   - likely under source/server/resources/
   - confirm actual paths from the repository

2. Request handling flow
   - how Create requests arrive
   - how Update requests arrive
   - which dispatcher calls FCNT-specific functions

3. FCNT Create handling
   - how cnd is validated
   - how custom attributes are parsed
   - how cs is calculated
   - how st is initialized or updated
   - how RTNode is created/updated
   - how DB insertion is performed
   - what response codes are returned on failure

4. FCNT Update handling
   - how existing FCNT is loaded
   - how custom attributes are modified
   - how cs is recalculated
   - how st is incremented or updated
   - how RTNode and DB are updated
   - what invalid updates are rejected

5. Validation
   - required attributes
   - invalid attribute handling
   - type checking
   - oneM2M response status codes

## Analysis method requested by the user

The user wants a study-friendly explanation.

When reporting findings, use this style:
- file name
- function name
- where it is called from
- input parameters
- first condition checked
- FCNT-specific logic
- validation logic
- RTNode effect
- DB effect
- failure response code
- short Korean explanation if possible

## Important instruction

Do not immediately modify the code.

First provide a report listing:
1. relevant FCNT files
2. relevant functions
3. request flow
4. suspected non-standard C parts
5. recommended next edits

Only after that, make small focused changes.