#include "az7798.h"

az7798::az7798() {

}

void az7798::begin() {


  state = asWait;
}

void az7798::handle() {
  if (state == asInit)
    return;



}
