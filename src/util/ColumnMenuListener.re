/* Document-level listeners for the table column menu. See MenuListener
 * for the shared machinery.
 *
 * Keys are handled at the document level (rather than via
 * tabindex+on_keydown on the menu div) because Hazel's editor (#page)
 * aggressively reclaims focus to the clipboard shim, which would
 * otherwise eat the menu's key events. */

include MenuListener.Make({
  let menu_class = "context-menu";
  let supports_keys = true;
  /* Off for the talk: the menu hangs below a probe's table, and scrolling
   * its selected item into view on every sync dragged the whole editor
   * down, where it stayed after the menu closed. */
  let scroll_into_view = false;
  let close_on_scroll = false;
});
