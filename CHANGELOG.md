# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## 0.4 - 2023-12-12

### Added

- User of the Manager API can provide an argument to be passed to the payload
- Message Queues to communicate between Linux and bare metal

### Changed

- Console output now includes CPU number and time is indicated in seconds

### Fixed

- Better reliability of detection of running monitor

## 0.3 - 2023-11-06

### Added

- New function overload `toString(DomainIndex)`

### Changed

- The payload runtime function `startPeriodicInterrupt` has been split into `setupPeriodicInterrupt` and
  `startPeriodicInterrupt`

## 0.2 - 2023-10-30

### Added

- New function `disableInterruptHandling`

### Changed

- Interrupt callbacks are now of type `std::function` (backwards-compatible change)
- The payload runtime function `configureAndEnableInterrupt` has been replaced by `setupInterruptHandling` and
  `enableInterruptHandling`

## 0.1 - 2023-10-25

### Added

- First versioned release
