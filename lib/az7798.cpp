#include "az7798.h"

az7798::az7798() {

}

void az7798::begin() {


  state = azWait;
}

void az7798::handle() {
  if (state == azInit)
    return;



}
