[@deriving sexp]
type t =
  | Rule
  | Case
  | Operand
  | BinOp({op_index: int})
  | NTuple({comma_indices: list(int)})
  | ListLit({comma_indices: list(int)})
  | SubBlock({hd_index: int});
