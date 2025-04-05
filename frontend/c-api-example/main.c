#include "leanstore/c.h"

void create_table(LeanStoreAdapterHandle* leanstore_adapter)
{

}

int main() {
   LeanStoreConfig config;

   leanstore_init_config(&config);
   config.ssd_path = "/home/devan/.leanstore/data";

   LeanStoreHandle* leanstore = leanstore_open(&config);
   LeanStoreAdapterHandle* leanstore_adapter = leanstore_get_adapter_u8();
   CRManagerHandle* crmanager = leanstore_get_cr_manager(leanstore);
   leanstore_create_table(crmanager, leanstore_adapter, leanstore, 0, "Example");
}