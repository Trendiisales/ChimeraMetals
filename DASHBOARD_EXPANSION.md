# ChimeraMetals Dashboard Expansion Plan

## Phase 1: Fix Zero Price Bug
- Add price validation in telemetry update
- Keep last valid price if current is 0
- Never show 0.00 prices on dashboard

## Phase 2: Expand TelemetrySnapshot Structure
Add to TelemetrySnapshot in TelemetryWriter.hpp:
- double xau_position
- double xag_position  
- double max_drawdown
- double sharpe_ratio
- int total_orders
- int total_fills
- double fill_rate
- double avg_win
- double avg_loss
- double win_rate
- char fix_quote_status[16]
- char fix_trade_status[16]
- int msg_rate
- int sequence_gaps

## Phase 3: Update Main.cpp
- Calculate RTT from Heartbeat responses
- Track position from ExecutionReports
- Calculate performance metrics
- Update telemetry with all new fields

## Phase 4: Update TelemetryServer API
- Return expanded JSON with all metrics

## Phase 5: Enhance Dashboard
- Add position display
- Add performance metrics panel
- Add FIX session status
- Add message rate graph
- Color code everything properly

