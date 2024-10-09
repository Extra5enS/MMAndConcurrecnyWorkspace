#include "memory_management/mark_sweep_gc/include/klass_table.h"
#include <string_view>
#include <regex>

KlassTable* KlassTable::inst_ = nullptr;

KlassTable* KlassTable::Create() {
    if(inst_ == nullptr) {
        inst_ = new KlassTable;
    }
    return inst_;
}

KlassTable* KlassTable::GetCurrent() {
    return inst_;
}

void KlassTable::Delete() {
    if(inst_ != nullptr) {
        delete inst_;
        inst_ = nullptr;
    }
}

KlassId KlassTable::ParseClass(const std::string& classString) {
    
}
