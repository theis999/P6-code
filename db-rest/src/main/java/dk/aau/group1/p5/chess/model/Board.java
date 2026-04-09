package dk.aau.group1.p5.chess.model;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonBackReference;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;

import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.FetchType;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.GenerationType;
import jakarta.persistence.Id;
import jakarta.persistence.JoinColumn;
import jakarta.persistence.ManyToOne;
import jakarta.persistence.OneToMany;
import jakarta.persistence.Table;

/** A Board can be played chess on
 * A board is owned by 1 user
 * A board can have multiple games
 */
@Entity
@Table(name = "boards")
@JsonIgnoreProperties(ignoreUnknown = true, value = {"id"}, allowGetters = true, allowSetters = true)
public class Board {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    @Column(name = "id", nullable = false)
    private Long id;

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
    }

    @ManyToOne(optional = false, fetch = FetchType.EAGER)
    @JoinColumn(name="user_id", nullable=false)
    @JsonBackReference
    private User user;


    public User getUser() {
        return user;
    }

    public void setUser(User user) {
        this.user = user;
    }


    @OneToMany(mappedBy = "board", fetch = FetchType.LAZY)
    @JsonIgnoreProperties(allowSetters = true)
    private List<Game> games;

    public List<Game> getGames(){
        return games;
    }

    public void setGames(List<Game> games) {
        this.games = games;
    }

    public Board() {}

    public Board(Long id, User user) {
        this.id = id;
        this.user = user;
    }


    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((id == null) ? 0 : id.hashCode());
        result = prime * result + ((user == null) ? 0 : user.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        Board other = (Board) obj;
        if (id == null) {
            if (other.id != null)
                return false;
        } else if (!id.equals(other.id))
            return false;
        if (user == null) {
            if (other.user != null)
                return false;
        } else if (!user.equals(other.user))
            return false;
        return true;
    }

    @Override
    public String toString() {
        return "Board [id=" + id + ", user=" + user + ", games="+ games +"}]";
    }

}
