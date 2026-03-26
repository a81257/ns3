# ns-3-UB Release Notes

## Release 1.1.0

**Release Date**: January 2026

### New Features

- **UNISON Multi-threaded Parallel Simulation**: Integrated UNISON framework for multi-threaded parallel simulation
- **DWRR Scheduling Algorithm**: Added Deficit Weighted Round Robin (DWRR) based inter-VL scheduling support on both network and data link layers
- **Adaptive Routing**: Implemented port-load-aware adaptive routing with configurable routing attributes
- **Deadlock Detection**: Added potential deadlock detection in UB switch and transport layer with enhanced packet arrival time tracking
- **CBFC Credit-Shared Mode**: Introduced CBFC credit-shared mode for more flexible flow control configuration


### Optimizations & Bug Fixes

- Optimized DWRR user configuration method
- Refactored buffer management architecture with unified VOQ management (dual-view with egress statistics)
- Enhanced routing table lookup process
- Improved queue management with byte-limit based egress queue management
- Fixed LDST CBFC compatibility issues
- Optimized flow control configuration interface
- Fixed TP removal and credit resumption at switch allocator
- Support for automatic TP generation without configuration files
- Support for useless TP removal optimization

---

## Release 1.0.0

Initial release of ns-3-UB simulator implementing the UnifiedBus Base Specification with comprehensive protocol stack support across function, transaction, transport, network, and data link layers.

**Key Features:**
- Complete UB protocol stack implementation
- Support for Load/Store and URMA programming interfaces
- Congestion control and flow control mechanisms
- Multi-path routing and load balancing
- QoS support with SP scheduling
- Credit-based flow control with CBFC support
