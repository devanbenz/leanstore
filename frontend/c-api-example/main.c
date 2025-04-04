#include "leanstore/c.h"

void create_table(void* data)
{
   int* i = (int*)data;
}

int main() {
   LeanStoreConfig config;

   leanstore_init_config(&config);
   config.ssd_path = "/home/devan/.leanstore/data";

   LeanStoreHandle* leanstore = leanstore_open(&config);

   CRManagerHandle* cr_manager = leanstore_get_cr_manager(leanstore);
   char* table_name = "ExampleTable";
   crm_schedule_job_sync(cr_manager, 0, create_table, table_name);
}