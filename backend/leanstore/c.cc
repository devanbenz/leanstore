#include "c.h"
#include "LeanStore.hpp"

#include "../../frontend/shared/GenericSchema.hpp"
#include "../../frontend/shared/LeanStoreAdapter.hpp"

using namespace leanstore;

// LeanStore C API

struct LeanStoreHandle {
   std::unique_ptr<LeanStore> store;
};

struct LeanStoreConfig
{

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

LeanStoreConfig* leanstore_init_config()
{

   LeanStoreConfig* config = new LeanStoreConfig();

   memset(config, 0, sizeof(LeanStoreConfig));
   config->ssd_path = "/home/devan/.leanstore/data";
   config->recover_file = "./leanstore.json";
   config->persist_file = "./leanstore.json";
   config->recover = 0;
   config->persist = 0;
   config->vi = 1;
   config->wal = 1;
   config->isolation_level = "repeatable_read";
   config->mv = 0;
   config->trunc = 0;
   config->falloc = 0;
   config->worker_threads = 1;
   config->wal_offset_gib = 0;

   return config;
}

LeanStoreHandle* leanstore_open(const LeanStoreConfig* config)
{
   if (!config)
      return nullptr;

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
   if (!handle)
      return;
   if (handle)
      delete handle;
}

// ----------------------------------------------------------------------------------------------

// LeanStore concurrency manager C API

struct CRManagerHandle {
   std::unique_ptr<cr::CRManager> cr_manager_inner;
};

CRManagerHandle* leanstore_get_cr_manager(LeanStoreHandle* leanstore_handle)
{
   if (!leanstore_handle)
      return nullptr;
   auto cr_manager = std::move(leanstore_handle->store->cr_manager);
   if (!cr_manager)
      return nullptr;

   CRManagerHandle* cr_manager_handle = new CRManagerHandle();
   cr_manager_handle->cr_manager_inner = std::move(cr_manager);

   return std::move(cr_manager_handle);
}

void leanstore_release_cr_manager(CRManagerHandle* cr_manager)
{
   if (!cr_manager)
      return;
   std::unique_ptr<cr::CRManager> cr_manager_inner = std::move(cr_manager->cr_manager_inner);
   if (!cr_manager_inner)
      return;
}


struct JobCtx {
   JobFunction fn;
   void* arg;
};

static void job_adapter(void* ctx)
{
   auto* job_ctx = static_cast<JobCtx*>(ctx);
   job_ctx->fn(job_ctx->arg);
   delete job_ctx;
}

void leanstore_crm_schedule_job_sync(CRManagerHandle* handle, uint64_t jobid, JobFunction fn, void* data)
{
   if (!handle)
      return;
   std::unique_ptr<cr::CRManager> cr_manager = std::move(handle->cr_manager_inner);
   if (!cr_manager)
      return;

   auto* ctx = new JobCtx{fn, data};

   const std::function cpp_fn = [ctx]() { job_adapter(ctx); };

   cr_manager->scheduleJobSync(jobid, cpp_fn);
}

// ----------------------------------------------------------------------------------------------

// LeanStoreAdapter C API

struct LeanStoreAdapterHandle
{
   LeanStoreAdapter<Relation<unsigned long, BytesPayload<8>>> leanstore_adapter;
};

LeanStoreAdapterHandle* leanstore_get_adapter_u8()
{
   using Key = u64;
   using Payload = BytesPayload<8>;
   using KVPair = Relation<Key, Payload>;

   LeanStoreAdapterHandle* adapter = new LeanStoreAdapterHandle();
   adapter->leanstore_adapter = LeanStoreAdapter<KVPair>();

   return adapter;
}

void leanstore_create_table(CRManagerHandle* handle, LeanStoreAdapterHandle* adapter, LeanStoreHandle* db_handle, uint64_t jobid, char* table_name)
{
   // TODO(DB): Make this generic. Right now I'm hard coding the sizes
   using Key = u64;
   using Payload = BytesPayload<8>;
   using KVPair = Relation<Key, Payload>;

   if (!handle)
      return;
   cr::CRManager* cr_manager = handle->cr_manager_inner.get();
   if (!cr_manager)
      return;
   if (!adapter)
      return;
   if (!table_name)
      return;
   if (!db_handle)
      return;

   auto db = db_handle->store.get();

   cr_manager->scheduleJobSync(jobid, [&] {adapter->leanstore_adapter = LeanStoreAdapter<KVPair>(*db, table_name); ;});
}

void leanstore_release_adapter(LeanStoreAdapterHandle* adapter)
{
   free(adapter);
}
