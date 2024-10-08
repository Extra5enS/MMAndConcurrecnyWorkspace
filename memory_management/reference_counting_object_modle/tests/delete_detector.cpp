#include "delete_detector.h"
#include <utility>

size_t DeleteDetector::deleteCount_ = 0;

DeleteDetector::~DeleteDetector() {
    deleteCount_++;
}

void DeleteDetector::SetDelete(Object<DeleteDetector> del) {
    delete_ = std::move(del);
}

void DeleteDetector::SetDeleteCount(size_t count) {
    deleteCount_ = count;
}

size_t DeleteDetector::GetDeleteCount() {
    return deleteCount_;
}