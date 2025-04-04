// leanstore_c_api.cpp
#include "c.h"
#include "LeanStore.hpp"  // Include the original LeanStore header

#include <cstring>
#include <memory>

using namespace leanstore;

struct CRManagerHandle
{
   cr::CRManager* cr_manager_inner;
};

struct LeanStoreHandle
{
    std::unique_ptr<LeanStore> store;
};

static void set_flags_from_config(const LeanStoreConfig* config)
{
    if (config->ssd_path) {
        FLAGS_ssd_path = config->ssd_path;
    }

    if (config->recover_file) {
        FLAGS_recover_file = config->recover_file;
    }

    if (config->persist_file) {
        FLAGS_persist_file = config->persist_file;
    }

    FLAGS_recover = config->recover;
    FLAGS_persist = config->persist;
    FLAGS_vi = config->vi;
    FLAGS_wal = config->wal;

    if (config->isolation_level) {
        FLAGS_isolation_level = config->isolation_level;
    }

    FLAGS_mv = config->mv;
    FLAGS_trunc = config->trunc;
    FLAGS_falloc = config->falloc;
    FLAGS_worker_threads = config->worker_threads;
    FLAGS_wal_offset_gib = config->wal_offset_gib;
}

void leanstore_init_config(LeanStoreConfig* config)
{
    if (!config) return;

    // Initialize with default values
    memset(config, 0, sizeof(LeanStoreConfig));
    config->ssd_path = "";
    config->recover_file = "./leanstore.json";
    config->persist_file = "./leanstore.json";
    config->recover = 0;
    config->persist = 0;
    config->vi = 0;
    config->wal = 1;
    config->isolation_level = "repeatable_read";
    config->mv = 0;
    config->trunc = 0;
    config->falloc = 0;
    config->worker_threads = 1;
    config->wal_offset_gib = 0;
}

struct JobCtx
{
   JobFunction fn;
   void* arg;
};

LeanStoreHandle* leanstore_open(const LeanStoreConfig* config)
{
    if (!config) return nullptr;

    try {
        set_flags_from_config(config);

        LeanStoreHandle* handle = new LeanStoreHandle();
        handle->store = std::make_unique<LeanStore>();
        return handle;
    } catch (const std::exception& e) {
        fprintf(stderr, "Error opening LeanStore: %s\n", e.what());
        return nullptr;
    }
}

void leanstore_close(LeanStoreHandle* handle)
{
   if (!handle) return;
   if (handle) delete handle;
}

CRManagerHandle* leanstore_get_cr_manager(LeanStoreHandle* leanstore_handle)
{
   if (!leanstore_handle) return nullptr;
   cr::CRManager* cr_manager = leanstore_handle->store->cr_manager.get();
   if (!cr_manager) return nullptr;

   CRManagerHandle* cr_manager_handle = new CRManagerHandle();
   cr_manager_handle->cr_manager_inner = cr_manager;

   return cr_manager_handle;
}

void leanstore_release_cr_manager(CRManagerHandle* cr_manager)
{
   if (!cr_manager) return;
   cr::CRManager* cr_manager_inner = cr_manager->cr_manager_inner;
   if (!cr_manager_inner) return;
   delete cr_manager_inner;
}

static void job_adapter(void* ctx)
{
   JobCtx* job_ctx = static_cast<JobCtx*>(ctx);
   job_ctx->fn(job_ctx->arg);
   delete job_ctx;
}

void crm_schedule_job_sync(CRManagerHandle* handle, uint64_t jobid, JobFunction fn, void* data)
{
   if (!handle) return;
   cr::CRManager* cr_manager = handle->cr_manager_inner;
   if (!cr_manager) return;

   JobCtx* ctx = new JobCtx{fn, data};

   std::function<void()> cpp_fn = [ctx]() {
      job_adapter(ctx);
   };

   cr_manager->scheduleJobSync(jobid, cpp_fn);
}
