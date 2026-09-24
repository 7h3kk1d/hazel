open Haz3lcore;
open Js_of_ocaml;

/*
 A minimal channel for driving Hazel from a host page.

 `Page.Update.t` already derives `sexp`, so the entire top-level action type is
 addressable as text. That makes the channel a parser and a dispatch — no new
 action constructors, no protocol, nothing to keep in sync as actions change.

 Installed on the global object at startup:

   window.hazelAction('(Globals (Set LiveTyping))')
   window.hazelAction('(Editors (Scratch RefreshStatics))')

 Loading a program is the one thing that is awkward to express as a sexp by
 hand, so it gets a second entry point that takes plain source text:

   window.hazelLoad('let x = 1 in\nx + ^^probe(x)')

 It accepts the same text as a .hz slide file — `^^probe`/`^^statics` triggers
 and all — and reuses the existing import action, so nothing new had to be
 added to the update type for it.

 Returns true if the action parsed and was scheduled, false otherwise; a parse
 failure logs the reason rather than throwing into the host.

 Intended for embedding Hazel in a slideshow or a docs page, where the host owns
 navigation and wants to drive the editor without simulating keystrokes. Actions
 go through the ordinary update loop, so history, statics and persistence all
 behave exactly as they would from the UI.

 This is deliberately smaller than the Patchwork protocol (see
 docs/embedding.md): there is no handshake, no outbound channel, and no
 conflict resolution. It is one-way, fire-and-forget, host-to-Hazel.
 */

let channel_name = "hazelAction";

let install = (schedule_action: Page.Update.t => unit): unit => {
  let dispatch = (s: Js.t(Js.js_string)): Js.t(bool) => {
    let text = Js.to_string(s);
    switch (Sexplib.Sexp.of_string(text) |> Page.Update.t_of_sexp) {
    | action =>
      schedule_action(action);
      Js._true;
    | exception (Sexplib.Conv.Of_sexp_error(exn, _)) =>
      print_endline(
        channel_name ++ ": not a Page.Update.t: " ++ Printexc.to_string(exn),
      );
      Js._false;
    | exception exn =>
      print_endline(channel_name ++ ": " ++ Printexc.to_string(exn));
      Js._false;
    };
  };

  /* The program to load, as a fresh scratchpad cell. The shape is exactly what
     Init.re builds for the empty scratchpad, with of_slide_text in place of the
     empty zipper so that probe and statics triggers in the text are honoured
     and leading indentation is stripped. */
  let persistent_of_text = (text: string): CellEditor.Model.persistent => {
    editor:
      text
      |> PersistentZipper.of_slide_text
      |> Editor.Model.mk_persistent(~root=Exp),
    result: EvalResult.Model.init |> EvalResult.Model.persist,
  };

  /* Round-tripping through a sexp looks redundant, but it means loading a
     program is the *existing* import action rather than a new one: whatever
     ScratchMode does on import — settings, statics, history — it does here
     too, and keeps doing if it changes.

     The import action is only handled in the scratch and documentation modes,
     and Hazel restores whichever mode was last used, so switch first rather
     than silently doing nothing in a tutorial or exercise. */
  let load = (s: Js.t(Js.js_string)): Js.t(bool) => {
    switch (
      s
      |> Js.to_string
      |> persistent_of_text
      |> CellEditor.Model.sexp_of_persistent
      |> Sexplib.Sexp.to_string
    ) {
    | data =>
      schedule_action(Page.Update.Editors(SwitchMode(Scratch)));
      schedule_action(
        Page.Update.Editors(Scratch(FinishImportScratchpad(Some(data)))),
      );
      Js._true;
    | exception exn =>
      print_endline("hazelLoad: " ++ Printexc.to_string(exn));
      Js._false;
    };
  };

  Js.Unsafe.set(
    Js.Unsafe.global,
    Js.string(channel_name),
    Js.wrap_callback(dispatch),
  );
  Js.Unsafe.set(
    Js.Unsafe.global,
    Js.string("hazelLoad"),
    Js.wrap_callback(load),
  );
};
