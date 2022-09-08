open Virtual_dom.Vdom;
open Node;
open Util.Web;

let cls_str = (ci: Haz3lcore.Statics.t): string =>
  switch (ci) {
  | Invalid(msg) => Haz3lcore.TermBase.show_parse_flag(msg)
  | InfoExp({cls, _}) => Haz3lcore.Term.UExp.show_cls(cls)
  | InfoPat({cls, _}) => Haz3lcore.Term.UPat.show_cls(cls)
  | InfoTyp({cls, _}) => Haz3lcore.Term.UTyp.show_cls(cls)
  | InfoRul({cls, _}) => Haz3lcore.Term.URul.show_cls(cls)
  };

let errorc = "error";
let happyc = "happy";
let infoc = "info";

let error_view = (err: Haz3lcore.Statics.error) =>
  switch (err) {
  | Multi =>
    div(~attr=clss([errorc, "err-multi"]), [text("⑂ Multi Hole")])
  | FreeVariable =>
    div(
      ~attr=clss([errorc, "err-free-variable"]),
      [text("⊥ Free Variable")],
    )
  | SynInconsistentBranches(tys) =>
    div(
      ~attr=clss([errorc, "err-inconsistent-branches"]),
      [text("≉ Branches:")] @ List.map(Type.view, tys),
    )
  | TypeInconsistent(ty_ana, ty_syn) =>
    div(
      ~attr=clss([errorc, "err-type-inconsistent"]),
      [Type.view(ty_ana), text("≉"), Type.view(ty_syn)],
    )
  };

let happy_view = (suc: Haz3lcore.Statics.happy) => {
  switch (suc) {
  | SynConsistent(ty_syn) =>
    div(
      ~attr=clss([happyc, "syn-consistent"]),
      [text("⇒"), Type.view(ty_syn)],
    )
  | AnaConsistent(ty_ana, ty_syn, _ty_join) when ty_ana == ty_syn =>
    div(
      ~attr=clss([happyc, "ana-consistent-equal"]),
      [text("⇐"), Type.view(ty_ana)],
    )
  | AnaConsistent(ty_ana, ty_syn, _ty_join) =>
    div(
      ~attr=clss([happyc, "ana-consistent"]),
      [text("⇐"), Type.view(ty_ana), text("≈"), Type.view(ty_syn)],
    )
  | AnaInternalInconsistent(ty_ana, _)
  | AnaExternalInconsistent(ty_ana, _) =>
    div(
      ~attr=clss([happyc, "ana-consistent-external"]),
      [
        div(
          ~attr=clss(["typ-view", "atom"]),
          [text("⇐"), div(~attr=clss(["typ-mod"]), [text("☆")])],
        ),
        Type.view(ty_ana),
      ],
    )
  };
};

let status_view = (err: Haz3lcore.Statics.error_status) => {
  switch (err) {
  | InHole(error) => error_view(error)
  | NotInHole(happy) => happy_view(happy)
  };
};

let term_tag = (is_err, sort) =>
  div(
    ~attr=
      Attr.many([
        clss(["term-tag", "term-tag-" ++ sort] @ (is_err ? [errorc] : [])),
      ]),
    [div(~attr=clss(["icon"]), [Icons.magnify]), text(sort)],
  );

let view_of_info = (ci: Haz3lcore.Statics.t): Node.t => {
  let is_err = Haz3lcore.Statics.is_error(ci);
  switch (ci) {
  | Invalid(msg) =>
    div(
      ~attr=clss([infoc, "unknown"]),
      [text("🚫 " ++ Haz3lcore.TermBase.show_parse_flag(msg))],
    )
  | InfoExp({mode, self, _}) =>
    let error_status = Haz3lcore.Statics.error_status(mode, self);
    div(
      ~attr=clss([infoc, "exp"]),
      [term_tag(is_err, "exp"), status_view(error_status)],
    );
  | InfoPat({mode, self, _}) =>
    let error_status = Haz3lcore.Statics.error_status(mode, self);
    div(
      ~attr=clss([infoc, "pat"]),
      [term_tag(is_err, "pat"), status_view(error_status)],
    );
  | InfoTyp({ty, _}) =>
    let ann = div(~attr=clss(["typ-view"]), [text(":")]);
    div(
      ~attr=clss([infoc, "typ"]),
      [term_tag(is_err, "typ"), ann, Type.view(ty)],
    );
  | InfoRul(_) =>
    div(
      ~attr=clss([infoc, "rul"]),
      [term_tag(is_err, "rul"), text("Rule")],
    )
  };
};

let cls_view = (ci: Haz3lcore.Statics.t): Node.t =>
  div(~attr=clss(["syntax-class"]), [text(cls_str(ci))]);

let id_view = (id): Node.t =>
  div(~attr=clss(["id"]), [text(string_of_int(id + 1))]);

let extra_view = (visible: bool, id: int, ci: Haz3lcore.Statics.t): Node.t =>
  div(
    ~attr=Attr.many([clss(["extra"] @ (visible ? ["visible"] : []))]),
    [id_view(id), cls_view(ci)],
  );

let toggle_context_and_print_ci = (~inject: Update.t => 'a, ci, _) => {
  print_endline(Haz3lcore.Statics.show(ci));
  switch (ci) {
  | InfoPat({mode, self, _})
  | InfoExp({mode, self, _}) =>
    Haz3lcore.Statics.error_status(mode, self)
    |> Haz3lcore.Statics.show_error_status
    |> print_endline
  | _ => ()
  };
  inject(Set(ContextInspector));
};

let inspector_view =
    (~inject, ~settings: Model.settings, id: int, ci: Haz3lcore.Statics.t)
    : Node.t =>
  div(
    ~attr=
      Attr.many([
        clss(
          ["cursor-inspector"]
          @ [Haz3lcore.Statics.is_error(ci) ? errorc : happyc],
        ),
        Attr.on_click(toggle_context_and_print_ci(~inject, ci)),
      ]),
    [
      extra_view(settings.context_inspector, id, ci),
      view_of_info(ci),
      CtxInspector.inspector_view(~settings, id, ci),
    ],
  );

let view =
    (
      ~inject,
      ~settings,
      index': option(int),
      info_map: Haz3lcore.Statics.map,
    ) => {
  switch (index') {
  | Some(index) =>
    switch (Haz3lcore.Id.Map.find_opt(index, info_map)) {
    | Some(ci) => inspector_view(~inject, ~settings, index, ci)
    | None =>
      div(
        ~attr=clss(["cursor-inspector"]),
        [
          div(~attr=clss(["icon"]), [Icons.magnify]),
          text("No CI for Index"),
        ],
      )
    }
  | None =>
    div(
      ~attr=clss(["cursor-inspector"]),
      [
        div(~attr=clss(["icon"]), [Icons.magnify]),
        text("No Indicated Index"),
      ],
    )
  };
};