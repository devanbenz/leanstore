#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

   typedef struct LeanStoreHandle LeanStoreHandle;
   typedef struct LeanStoreAdapterHandle LeanStoreAdapterHandle;
   typedef struct CRManagerHandle CRManagerHandle;
   typedef void (*JobFunction)(void* fn);
   typedef struct {
      const char* ssd_path;
      const char* recover_file;
      const char* persist_file;
      int recover;
      int persist;
      int vi;
      int wal;
      const char* isolation_level;
      int mv;
      int trunc;
      int falloc;
      int worker_threads;
      int wal_offset_gib;
   } LeanStoreConfig;


   void leanstore_init_config(LeanStoreConfig* config);

   LeanStoreHandle* leanstore_open(const LeanStoreConfig* config);

   void leanstore_close(LeanStoreHandle* handle);

   CRManagerHandle* leanstore_get_cr_manager(LeanStoreHandle* handle);

   void leanstore_crm_schedule_job_sync(CRManagerHandle* handle, uint64_t jobid, JobFunction job, void* fn);

   void leanstore_release_cr_manager(CRManagerHandle* cr_manager);

   LeanStoreAdapterHandle* leanstore_get_adapter_u8();

   void leanstore_release_adapter(LeanStoreAdapterHandle* adapter);

   void leanstore_create_table(CRManagerHandle* handle, LeanStoreAdapterHandle* adapter, LeanStoreHandle* db_handle, uint64_t jobid, char* table_name);

#ifdef __cplusplus
}
#endif
