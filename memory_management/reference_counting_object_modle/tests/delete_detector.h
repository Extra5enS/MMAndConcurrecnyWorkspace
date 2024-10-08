#ifndef MEMORY_MANAGEMENT_REFERENCE_COUNTING_GC_TEST_DELETE_DETECTOR_H
#define MEMORY_MANAGEMENT_REFERENCE_COUNTING_GC_TEST_DELETE_DETECTOR_H

#include "memory_management/reference_counting_object_modle/include/object_modle.h"
#include "base/macros.h"

class DeleteDetector {
public:
    DeleteDetector() = default;
    NO_COPY_SEMANTIC(DeleteDetector);
    DEFAULT_MOVE_SEMANTIC(DeleteDetector);
    ~DeleteDetector();

    void SetDelete(Object<DeleteDetector> del);

    static void SetDeleteCount(size_t count);
    static size_t GetDeleteCount();

private:
    Object<DeleteDetector> delete_ {nullptr};

    static size_t deleteCount_;
};

#endif  // MEMORY_MANAGEMENT_REFERENCE_COUNTING_GC_TEST_DELETE_DETECTOR_H