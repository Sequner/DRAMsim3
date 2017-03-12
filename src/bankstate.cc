#include "bankstate.h"
#include <iostream>
#include <algorithm>

using namespace std;

BankState::BankState(int rank, int bankgroup, int bank) :
    state_(State::CLOSED),
    timing_(int(CommandType::SIZE)),
    open_row_(-1),
    row_hit_count_(0),
    refresh_waiting_(false),
    rank_(rank),
    bankgroup_(bankgroup),
    bank_(bank)
{
    timing_[int(CommandType::READ)] = 0;
    timing_[int(CommandType::READ_PRECHARGE)] = 0;
    timing_[int(CommandType::WRITE)] = 0;
    timing_[int(CommandType::WRITE_PRECHARGE)] = 0;
    timing_[int(CommandType::ACTIVATE)] = 0;
    timing_[int(CommandType::PRECHARGE)] = 0;
    timing_[int(CommandType::REFRESH)] = 0;
    timing_[int(CommandType::SELF_REFRESH_ENTER)] = 0;
    timing_[int(CommandType::SELF_REFRESH_EXIT)] = 0;

    printf("Bankstate object created with rank = %d, bankgroup = %d, bank = %d\n", rank_, bankgroup_, bank_);
}

CommandType BankState::GetRequiredCommandType(const Command& cmd) {
    switch(cmd.cmd_type_) {
        case CommandType::READ:
            switch(state_) {
                case State::CLOSED:
                    return CommandType::ACTIVATE;
                case State::OPEN:
                    return cmd.row_ == open_row_ ? CommandType::READ : CommandType::PRECHARGE;
                case State::SELF_REFRESH:
                    return CommandType::SELF_REFRESH_EXIT;
                default:
                    exit(-1);
            }
            case CommandType::READ_PRECHARGE:
            switch(state_) {
                case State::CLOSED:
                    return CommandType::ACTIVATE;
                case State::OPEN:
                    return cmd.row_ == open_row_ ? CommandType::READ_PRECHARGE : CommandType::PRECHARGE;
                case State::SELF_REFRESH:
                    return CommandType::SELF_REFRESH_EXIT;
                default:
                    exit(-1);
            }
        case CommandType::WRITE:
            switch(state_) {
                case State::CLOSED:
                    return CommandType::ACTIVATE;
                case State::OPEN:
                    return cmd.row_ == open_row_ ? CommandType::WRITE : CommandType::PRECHARGE;
                case State::SELF_REFRESH:
                    return CommandType::SELF_REFRESH_EXIT;
                default:
                    exit(-1);
            }
            case CommandType::WRITE_PRECHARGE:
            switch(state_) {
                case State::CLOSED:
                    return CommandType::ACTIVATE;
                case State::OPEN:
                    return cmd.row_ == open_row_ ? CommandType::WRITE_PRECHARGE : CommandType::PRECHARGE;
                case State::SELF_REFRESH:
                    return CommandType::SELF_REFRESH_EXIT;
                default:
                    exit(-1);
            }
            case CommandType::REFRESH_BANK:
            switch(state_) {
                case State::CLOSED:
                    return CommandType::REFRESH_BANK;
                case State::OPEN:
                    return CommandType::PRECHARGE;
                case State::SELF_REFRESH:
                    return CommandType::SELF_REFRESH_EXIT;
                default:
                    exit(-1);
            }
        case CommandType::REFRESH:
            switch(state_) {
                case State::CLOSED:
                    return CommandType::REFRESH;
                case State::OPEN:
                    return CommandType::PRECHARGE;
                case State::SELF_REFRESH:
                    return CommandType::SELF_REFRESH_EXIT;
                default:
                    exit(-1);
            }
        case CommandType::SELF_REFRESH_ENTER:
            switch(state_) {
                case State::CLOSED:
                    return CommandType::SELF_REFRESH_ENTER;
                case State::OPEN:
                    return CommandType::PRECHARGE;
                case State::SELF_REFRESH:
                default:
                    exit(-1);
            }
        case CommandType::SELF_REFRESH_EXIT:
            switch(state_) {
                case State::SELF_REFRESH:
                    return CommandType::SELF_REFRESH_EXIT;
                case State::CLOSED:
                case State::OPEN:
                default:
                    exit(-1);
            }
        // ACTIVATE and PRECHARGE are on demand commands
        // They will not be scheduled on their own.
        case CommandType::ACTIVATE:
        case CommandType::PRECHARGE:
        default:
            exit(-1);
    }
}

void BankState::UpdateState(const Command& cmd) {
    switch(state_) {
        case State::OPEN:
            switch(cmd.cmd_type_) {
                case CommandType::READ:
                case CommandType::WRITE:
                    row_hit_count_++;
                    break;
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE_PRECHARGE:
                case CommandType::PRECHARGE:
                    state_ = State::CLOSED;
                    open_row_ = -1;
                    row_hit_count_ = 0;
                    break;
                case CommandType::ACTIVATE:
                case CommandType::REFRESH:
                case CommandType::REFRESH_BANK:
                case CommandType::SELF_REFRESH_ENTER:
                case CommandType::SELF_REFRESH_EXIT:
                default:
                    exit(-1);
            }
            break;
        case State::CLOSED:
            switch(cmd.cmd_type_) {
                case CommandType::REFRESH:
                case CommandType::REFRESH_BANK:
                    break;
                case CommandType::ACTIVATE:
                    state_ = State::OPEN;
                    open_row_ = cmd.row_;
                    break;
                case CommandType::SELF_REFRESH_ENTER:
                    state_ = State::SELF_REFRESH;
                    break;
                case CommandType::READ:
                case CommandType::WRITE:
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE_PRECHARGE:
                case CommandType::PRECHARGE:
                case CommandType::SELF_REFRESH_EXIT:
                default:
                    exit(-1);
            }
            break;
        case State::SELF_REFRESH:
            switch(cmd.cmd_type_) {
                case CommandType::SELF_REFRESH_EXIT:
                    state_ = State::CLOSED;
                case CommandType::READ:
                case CommandType::WRITE:
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE_PRECHARGE:
                case CommandType::ACTIVATE:
                case CommandType::PRECHARGE:
                case CommandType::REFRESH:
                case CommandType::REFRESH_BANK:
                case CommandType::SELF_REFRESH_ENTER:
                default:
                    exit(-1);
            }
        default:
            exit(-1);
    }
    return;
}

void BankState::UpdateTiming(CommandType cmd_type, long time) {
    timing_[int(cmd_type)] = max(timing_[int(cmd_type)], time);
    return;
}

bool BankState::IsReady(CommandType cmd_type, long time) {
    return time >= timing_[int(cmd_type)];
}


void BankState::UpdateRefreshWaitingStatus(bool status) {
    refresh_waiting_ = status;
    return;
}


bool BankState::IsRefreshWaiting() const {
    return refresh_waiting_;
}

bool BankState::IsRowOpen() const {
    return state_ == State::OPEN;
}

int BankState::OpenRow() const {
    return open_row_;
}

int BankState::RowHitCount() const {
    return row_hit_count_;
}