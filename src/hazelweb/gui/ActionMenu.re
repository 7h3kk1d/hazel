open Virtual_dom.Vdom;
open Node;

type menu_entry = {
  id: string,
  label: string,
  shortcut: option(string),
  action: ModelAction.t,
};

let menu_entries: list(menu_entry) = [
  {
    id: "serialize-to-console",
    label: "Serialize to console",
    shortcut: Some("Ctrl-S"),
    action: ModelAction.SerializeToConsole,
  },
];

let dropdown_option = (~inject, {id, label, shortcut, action}: menu_entry) => {
  let shortcut_view =
    switch (shortcut) {
    | None => []
    | Some(s) => [div([Attr.classes(["shortcut"])], [text(s)])]
    };
  li(
    [Attr.id(id), Attr.on_click(_ => inject(action))],
    [text(label)] @ shortcut_view,
  );
};

let dropdown_options = (~inject) =>
  List.map(dropdown_option(~inject), menu_entries);

let dropdown = (~inject: ModelAction.t => Ui_event.t, ~model as _: Model.t) => {
  create(
    "details",
    [],
    [
      create("summary", [], [text("☰")]),
      ul([Attr.classes(["dropdown-content"])], dropdown_options(~inject)),
    ],
  );
};

let view = (~inject: ModelAction.t => Ui_event.t, ~model: Model.t) =>
  div([Attr.classes(["dropdown"])], [dropdown(~inject, ~model)]);
