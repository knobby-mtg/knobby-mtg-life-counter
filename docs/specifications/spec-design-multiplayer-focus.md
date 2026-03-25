---
title: Multiplayer-First Product Refinement
version: 1.0
date_created: 2026-03-25
last_updated: 2026-03-25
owner: Knobby MTG Life Counter Team
tags: [design, app]
---

# Introduction

This specification details the product direction changes for the Knobby MTG Life Counter. The goal is to refine the product by removing single-player functionality and auxiliary features to focus exclusively on multiplayer Commander gameplay.

## 1. Purpose & Scope

The purpose of this specification is to formally document the removal of features that are out of scope for the multiplayer-focused product. The scope covers the removal of all single-player modes, the game timer, and the d20 roller utility. This spec will guide the necessary code removal and updates to other documentation.

## 2. Definitions

- **MP:** Multiplayer
- **SP:** Single-player

## 3. Requirements, Constraints & Guidelines

- **REQ-001**: The product shall be refactored to remove all entry points and UI elements related to single-player life tracking.
- **REQ-002**: The product shall be refactored to remove the turn timer and associated UI and logic.
- **REQ-003**: The product shall be refactored to remove the d20 rolling utility.
- **CON-001**: The core multiplayer functionality must not be negatively impacted by these changes.
- **GUD-001**: All related documentation, including `product-requirements.md`, should be updated to reflect these changes once the refactoring is complete.

## 4. Interfaces & Data Contracts

There are no new interfaces or data contracts. This specification is for the removal of existing features.

## 5. Acceptance Criteria

- **AC-001**: Given the device has booted, When the user navigates the application, Then there shall be no option to enter a single-player mode. The device should load into a multiplayer game.
- **AC-002**: Given the device is in a multiplayer game, When the user interacts with the UI, Then there shall be no option to start or view a game timer.
- **AC-003**: Given the device is in a multiplayer game, When the user navigates the application, Then there shall be no option to access a d20 roller.

## 6. Test Automation Strategy

- **Test Levels**: Existing unit and integration tests for the removed features should be deleted. New tests may be required to ensure that the removal of these features has not broken multiplayer functionality.
- **CI/CD Integration**: The CI/CD pipeline should continue to run all remaining tests.

## 7. Rationale & Context

As stated in the `README.md`, this fork of the project is intended to focus solely on multiplayer Commander gameplay. The removal of single-player and other auxiliary features simplifies the user experience and codebase, allowing for more focused development on the core multiplayer features.

## 8. Dependencies & External Integrations

No new dependencies are introduced. This work involves removing code and dependencies related to the features being removed.

## 9. Examples & Edge Cases

N/A

## 10. Validation Criteria

- A code review confirms the removal of all files and code paths related to single-player mode, timer, and d20 roller.
- The application builds and runs without errors.
- Manual testing confirms that the UI no longer presents the removed features.

## 11. Related Specifications / Further Reading

- [product-requirements.md](./product-requirements.md)
- [functional-spec.md](./functional-spec.md)
