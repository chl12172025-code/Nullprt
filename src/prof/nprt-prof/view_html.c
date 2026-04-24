#include <stdio.h>

void prof_emit_interactive_html_intro(void) {
  printf("<script>\n");
  printf("function zoomFrame(id){const el=document.getElementById(id);if(el){el.style.stroke='red';el.style.strokeWidth='2';}}\n");
  printf("function searchFlame(){const q=document.getElementById('q').value.toLowerCase();document.querySelectorAll('[data-name]').forEach(e=>{e.style.opacity=e.dataset.name.includes(q)?'1':'0.2';});}\n");
  printf("</script>\n");
}
