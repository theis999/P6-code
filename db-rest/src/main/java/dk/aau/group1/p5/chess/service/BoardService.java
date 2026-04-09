package dk.aau.group1.p5.chess.service;

import java.util.List;
import java.util.Optional;

import org.springframework.stereotype.Service;

import dk.aau.group1.p5.chess.model.Board;
import dk.aau.group1.p5.chess.repository.BoardRepository;

@Service
public class BoardService {
    private final BoardRepository repository;

    public BoardService(BoardRepository boardRepository)
    {
        repository = boardRepository;
    }

    public List<Board> getAll(){
        return repository.findAll();
    }

    public Optional<Board> get(long id) {
        return repository.findById(id);
    }

    public Board save(Board board) {
        assert(board != null);
        
        return repository.save(board);
    }
}
